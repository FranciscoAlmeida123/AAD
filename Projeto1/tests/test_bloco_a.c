//
// test_bloco_a.c --- tests of the file layouts, build_candidate, search_scalar and write_result
//
//   bin/test_bloco_a      (run from Projeto1; uses sha1sum and inputs/, examples/)
//
// For each file: the SHA1 computed from midstate + tail (build_candidate) must equal the
// sha1sum of the file written by write_result, for several counters; only the expected bytes
// may change; search_scalar must find exactly the counters a brute force with build_candidate finds.
//

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "aad_engines.h"
#include "aad_files.h"
#include "aad_mutate.h"

static int failures = 0, checks = 0;
static char tmp_dir[64];

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        checks++;                                                          \
        if (!(cond)) {                                                     \
            failures++;                                                    \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);           \
            fprintf(stderr, __VA_ARGS__);                                  \
            fputc('\n', stderr);                                           \
        }                                                                  \
    } while (0)

static void hex_digest(const u32_t h[5], char out[41])
{
    for (int i = 0; i < 5; i++)
        sprintf(out + 8 * i, "%08x", h[i]);
}

static int sha1sum_file(const char *path, char out[41])
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "sha1sum '%s'", path);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL)
        return -1;
    int ok = fread(out, 1, 40, fp) == 40;
    out[40] = '\0';
    pclose(fp);
    return ok ? 0 : -1;
}

static u08_t *slurp(const char *path, long *len)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;
    fseek(fp, 0, SEEK_END);
    *len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    u08_t *p = malloc((size_t)*len + 1);
    if (fread(p, 1, (size_t)*len, fp) != (size_t)*len) { free(p); p = NULL; }
    fclose(fp);
    return p;
}

static void write_bytes(const char *path, const void *data, size_t len)
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL || fwrite(data, 1, len, fp) != len) { perror(path); exit(1); }
    fclose(fp);
}

// independent PNG check: every chunk CRC up to IEND recomputed bit by bit (no shared code)
static u32_t be32(const u08_t *p)
{
    return (u32_t)p[0] << 24 | (u32_t)p[1] << 16 | (u32_t)p[2] << 8 | (u32_t)p[3];
}

static int png_crcs_ok(const u08_t *f, long len)
{
    for (long off = 8; off + 12 <= len;) {
        long n = (long)be32(f + off);
        if (off + 12 + n > len)
            return 0;
        u32_t c = 0xFFFFFFFFu;
        for (long i = 0; i < n + 4; i++) {
            c ^= f[off + 4 + i];
            for (int k = 0; k < 8; k++)
                c = (c & 1u) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
        }
        if ((c ^ 0xFFFFFFFFu) != be32(f + off + 8 + n))
            return 0;
        if (memcmp(f + off + 4, "IEND", 4) == 0)
            return 1;
        off += 12 + n;
    }
    return 0;
}

// sha1_buffer against sha1sum on random buffers of every length 0..200 and on real files
static void test_sha1_buffer(const char *const *files, int n_files)
{
    char path[128], a[41], b[41];
    u08_t data[200];
    u32_t h[5];

    snprintf(path, sizeof(path), "%s/rand.bin", tmp_dir);
    srand(12345);
    for (int len = 0; len <= 200; len++) {
        for (int i = 0; i < len; i++)
            data[i] = (u08_t)rand();
        write_bytes(path, data, (size_t)len);
        sha1_buffer(data, (u64_t)len, h);
        hex_digest(h, a);
        CHECK(sha1sum_file(path, b) == 0 && strcmp(a, b) == 0, "sha1_buffer length %d: %s vs %s", len, a, b);
    }
    for (int f = 0; f < n_files; f++) {
        long len;
        u08_t *p = slurp(files[f], &len);
        CHECK(p != NULL, "cannot read %s", files[f]);
        if (p == NULL)
            continue;
        sha1_buffer(p, (u64_t)len, h);
        hex_digest(h, a);
        CHECK(sha1sum_file(files[f], b) == 0 && strcmp(a, b) == 0, "sha1_buffer %s: %s vs %s", files[f], a, b);
        free(p);
    }
}

// full check of one input; expect_blocks = 0 means "do not check n_blocks"
static void test_file(const char *path, file_type_t type, u32_t expect_blocks)
{
    static const u64_t counters[] = { 0, 1, 63, 64, 4095, 123456789, 0x0123456789ABCDEFul, 0xFFFFFFFFFFFFul, ~0ul };
    search_template_t t;
    char out[160], out0[160], a[41], b[41];
    u32_t w[32], h[5];

    if (make_template(path, type, &t) != 0) {
        CHECK(0, "make_template(%s) failed", path);
        return;
    }
    printf("  %-44s n_blocks=%u var_off=%3u total_len=%lu\n", path, t.n_blocks, t.var_off, t.total_len);
    if (expect_blocks)
        CHECK(t.n_blocks == expect_blocks, "%s: n_blocks %u, expected %u", path, t.n_blocks, expect_blocks);
    snprintf(out0, sizeof(out0), "%s/c0.out", tmp_dir);
    snprintf(out, sizeof(out), "%s/cx.out", tmp_dir);
    CHECK(write_result(path, &t, 0, out0) == 0, "%s: write_result counter 0", path);
    long len0, lenx;
    u08_t *f0 = slurp(out0, &len0);
    CHECK(f0 != NULL && (u64_t)len0 == t.total_len, "%s: counter-0 file length", path);

    for (size_t k = 0; k < sizeof(counters) / sizeof(counters[0]); k++) {
        u64_t c = counters[k];
        build_candidate(&t, c, w);
        sha1_candidate(&t, w, h);
        hex_digest(h, a);
        CHECK(write_result(path, &t, c, out) == 0, "%s: write_result counter %lu", path, c);
        CHECK(sha1sum_file(out, b) == 0 && strcmp(a, b) == 0, "%s counter %lu: candidate %s vs file %s", path, c, a, b);
        // only the variable bytes (and the PNG CRC) may differ from the counter-0 file
        u08_t *fx = slurp(out, &lenx);
        if (type == FT_PNG)
            CHECK(fx != NULL && png_crcs_ok(fx, lenx), "%s counter %lu: bad PNG chunk CRC", path, c);
        if (f0 != NULL && fx != NULL && lenx == len0) {
            u64_t v0 = t.prefix_len + t.var_off, v1 = v0 + t.var_len;
            u64_t c0 = t.prefix_len + t.crc_off, c1 = (type == FT_PNG) ? c0 + 4 : c0;
            for (long i = 0; i < len0; i++)
                if (f0[i] != fx[i] && !((u64_t)i >= v0 && (u64_t)i < v1) && !((u64_t)i >= c0 && (u64_t)i < c1)) {
                    CHECK(0, "%s counter %lu: byte %ld changed outside the variable area", path, c, i);
                    break;
                }
        } else {
            CHECK(0, "%s: length changed with counter %lu", path, c);
        }
        free(fx);
    }
    free(f0);

    // search_scalar (specialised) == brute force with build_candidate, mask 0xFF, across 64-boundaries
    const u64_t first = (1ul << 40) - 1000, count = 6000;
    u64_t found[64], brute[64];
    u32_t n_found = 0, n_brute = 0;
    t.h4_mask = 0xFFu;
    u64_t updates = search_scalar(&t, first, count, found, 64, &n_found);
    CHECK(updates == count * t.n_blocks, "%s: search_scalar returned %lu updates", path, updates);
    for (u64_t c = first; c < first + count; c++) {
        build_candidate(&t, c, w);
        sha1_candidate(&t, w, h);
        if ((h[4] & 0xFFu) == 0 && n_brute < 64)
            brute[n_brute++] = c;
    }
    CHECK(n_found == n_brute && memcmp(found, brute, n_brute * sizeof(u64_t)) == 0,
          "%s: search_scalar found %u, brute force %u", path, n_found, n_brute);
    // MVP: the first solution written to a file has a sha1sum ending in "00"
    if (n_found > 0) {
        CHECK(write_result(path, &t, found[0], out) == 0 && sha1sum_file(out, b) == 0 && strcmp(b + 38, "00") == 0,
              "%s: solution file sha1 %s", path, b);
    }
}

// synthetic PPM whose final length L has a given L mod 64
static void make_ppm(const char *path, int target_mod_lo, int target_mod_hi)
{
    char hdr[32];
    for (int wd = 4; wd < 200; wd++) {
        int n = snprintf(hdr, sizeof(hdr), "P6\n%d 4\n255\n", wd);
        long len = n + 3L * wd * 4;
        if (len % 64 >= target_mod_lo && len % 64 <= target_mod_hi) {
            u08_t *p = malloc((size_t)len);
            memcpy(p, hdr, (size_t)n);
            for (long i = n; i < len; i++)
                p[i] = (u08_t)(i * 37 + 11);
            write_bytes(path, p, (size_t)len);
            free(p);
            return;
        }
    }
    fprintf(stderr, "make_ppm: no size found\n");
    exit(1);
}

static void make_text(const char *path, const char *text)
{
    write_bytes(path, text, strlen(text));
}

int main(void)
{
    char p[16][128];
    snprintf(tmp_dir, sizeof(tmp_dir), "/tmp/aad_test_%d", (int)getpid());
    if (snprintf(p[0], sizeof(p[0]), "mkdir -p %s", tmp_dir) > 0 && system(p[0]) != 0)
        return 1;

    static const char *const real_files[] = {
        "inputs/aad_a1.pdf", "inputs/aad_a1.png", "inputs/aad_a1.ppm", "inputs/aad_a1.html", "inputs/aad_a1.txt",
        "examples/deti_4_2_37_original.pdf", "examples/example_frame_001_original.png",
        "examples/example_frame_001.ppm",
    };
    printf("sha1_buffer vs sha1sum\n");
    test_sha1_buffer(real_files, 8);

    printf("templates, candidates and written files\n");
    test_file("inputs/aad_a1.pdf", FT_PDF, 1);
    test_file("inputs/aad_a1.png", FT_PNG, 1);
    test_file("inputs/aad_a1.ppm", FT_PPM, 0);
    test_file("inputs/aad_a1.html", FT_HTML, 1);
    test_file("inputs/aad_a1.txt", FT_TXT, 1);
    test_file("examples/deti_4_2_37_original.pdf", FT_PDF, 1);
    test_file("examples/example_frame_001_original.png", FT_PNG, 1);
    test_file("examples/example_frame_001.ppm", FT_PPM, 0);

    // edge cases
    snprintf(p[1], 128, "%s/one_block.ppm", tmp_dir);    make_ppm(p[1], 48, 55); test_file(p[1], FT_PPM, 1);
    snprintf(p[2], 128, "%s/split.ppm", tmp_dir);        make_ppm(p[2], 0, 47);  test_file(p[2], FT_PPM, 2);
    snprintf(p[3], 128, "%s/pad_block.ppm", tmp_dir);    make_ppm(p[3], 56, 63); test_file(p[3], FT_PPM, 2);
    snprintf(p[4], 128, "%s/crlf.pdf", tmp_dir);
    make_text(p[4], "%PDF-1.4\r\n1 0 obj\r\n<<>>\r\nendobj\r\nxref\r\n0 1\r\ntrailer\r\n<<>>\r\nstartxref\r\n27\r\n%%EOF\r\n");
    test_file(p[4], FT_PDF, 1);
    snprintf(p[5], 128, "%s/inline.pdf", tmp_dir);
    make_text(p[5], "%PDF-1.4\nobj endobj startxref\n9\n%%EOF\n"); // no end of line before startxref
    test_file(p[5], FT_PDF, 1);
    snprintf(p[6], 128, "%s/junk.pdf", tmp_dir);         // |S| = 52 -> var ends at a block boundary
    make_text(p[6], "%PDF-1.4\nxref\nstartxref\n9\n%%EOF\n0123456789012345678901234567890");
    test_file(p[6], FT_PDF, 2);
    snprintf(p[7], 128, "%s/too_much.pdf", tmp_dir);     // |S| > 55 -> make_template must fail
    make_text(p[7], "%PDF-1.4\nstartxref\n9\n%%EOF\n0123456789012345678901234567890123456789012345678901234567890");
    search_template_t t;
    CHECK(make_template(p[7], FT_PDF, &t) != 0, "too_much.pdf should fail");
    snprintf(p[8], 128, "%s/no_newline.txt", tmp_dir);  make_text(p[8], "last line without newline"); test_file(p[8], FT_TXT, 1);
    snprintf(p[9], 128, "%s/empty.txt", tmp_dir);       make_text(p[9], "");                          test_file(p[9], FT_TXT, 1);
    snprintf(p[10], 128, "%s/nostartxref.pdf", tmp_dir); make_text(p[10], "%PDF-1.4\n%%EOF\n");
    CHECK(make_template(p[10], FT_PDF, &t) != 0, "PDF without startxref should fail");
    snprintf(p[11], 128, "%s/bad.ppm", tmp_dir);        make_text(p[11], "P6\n4 4\n65535\n0123");
    CHECK(make_template(p[11], FT_PPM, &t) != 0, "PPM with maxval > 255 should fail");

    // PNG with 30 bytes after IEND: |S| = 46 -> n_blocks = 2 (CRC in the second block)
    {
        long n;
        u08_t *png = slurp("inputs/aad_a1.png", &n);
        u08_t *junk = malloc((size_t)n + 30);
        memcpy(junk, png, (size_t)n);
        memset(junk + n, 'J', 30);
        snprintf(p[12], 128, "%s/junk.png", tmp_dir);
        write_bytes(p[12], junk, (size_t)n + 30);
        test_file(p[12], FT_PNG, 2);
        // malformed: first chunk length 0xFFFFFFF0 -> no IEND found, no crash
        memcpy(junk + 8, "\xFF\xFF\xFF\xF0", 4);
        snprintf(p[13], 128, "%s/bad_len.png", tmp_dir);
        write_bytes(p[13], junk, (size_t)n);
        CHECK(make_template(p[13], FT_PNG, &t) != 0, "PNG with a huge chunk length should fail");
        free(png);
        free(junk);
    }
    snprintf(p[15], 128, "%s/255x.ppm", tmp_dir); make_text(p[15], "P6 4 4 255X012345678901234567890123456789012345678901234567");
    CHECK(make_template(p[15], FT_PPM, &t) != 0, "PPM header 255X should fail");

    file_type_t ft;
    CHECK(file_type_from_path("a/b.HTM", &ft) == 0 && ft == FT_HTML, "extension .HTM");
    CHECK(file_type_from_path("x.jpg", &ft) != 0, "extension .jpg must be rejected");

    snprintf(p[0], sizeof(p[0]), "rm -rf %s", tmp_dir);
    if (system(p[0]) != 0)
        fprintf(stderr, "warning: could not remove %s\n", tmp_dir);
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
