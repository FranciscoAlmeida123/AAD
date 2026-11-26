//
// aad_files.c --- file layouts, search template (midstate + tail) and result writer
//
// The final file is [fixed prefix][fill][variable bytes][fixed suffix S]. For text types the
// fill length f in [0,63] is chosen so that the final length L has L mod 64 = 55: the variable
// bytes, S and the 9 SHA1 padding bytes then share the last block (n_blocks = 1) when |S| <= 44.
// If 44 < |S| <= 55 the variable bytes end at a block boundary instead (n_blocks = 2).
// PPM keeps its size; only the LSB of the last 48 colour bytes changes (read with aad_ppm.h).
//
// Used by aad_modify, aad_server and the tests (make_template, write_result).
//

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "aad_files.h"
#include "aad_mutate.h"
#include "aad_sha1_cpu.h"
#include "aad_png_crc.h"
#include "aad_ppm.h"

typedef struct {
    u08_t *buf;        // final file for counter 0
    u64_t len;         // final file length
    u64_t var_pos;     // file offset of the first variable byte
    u64_t crc_pos;     // PNG: file offset of the tEXt CRC
    u32_t crc_prefix;  // PNG: CRC register before the variable bytes
} layout_t;

static const char *const type_names[] = { "pdf", "png", "ppm", "html", "txt" };

const char *file_type_name(file_type_t type)
{
    return ((unsigned)type < 5u) ? type_names[type] : "?";
}

int file_type_from_path(const char *path, file_type_t *type)
{
    const char *dot = strrchr(path, '.');

    if (dot == NULL)
        return -1;
    if (strcasecmp(dot, ".pdf") == 0) *type = FT_PDF;
    else if (strcasecmp(dot, ".png") == 0) *type = FT_PNG;
    else if (strcasecmp(dot, ".ppm") == 0) *type = FT_PPM;
    else if (strcasecmp(dot, ".html") == 0 || strcasecmp(dot, ".htm") == 0) *type = FT_HTML;
    else if (strcasecmp(dot, ".txt") == 0) *type = FT_TXT;
    else return -1;
    return 0;
}

//
// SHA1 helpers (the original macro, through sha1_update of aad_sha1_cpu.h)
//

static void pack_block(const u08_t *p, u32_t w[16])
{
    for (int i = 0; i < 16; i++)
        w[i] = (u32_t)p[4 * i] << 24 | (u32_t)p[4 * i + 1] << 16 | (u32_t)p[4 * i + 2] << 8 | (u32_t)p[4 * i + 3];
}

static void sha1_init(u32_t h[5])
{
    h[0] = initial_H0; h[1] = initial_H1; h[2] = initial_H2; h[3] = initial_H3; h[4] = initial_H4;
}

// appends the SHA1 padding of a message of total length len to the last len % 64 bytes in p
// (p has room for 128 bytes); returns the number of padded bytes (64 or 128)
static u32_t sha1_pad(u08_t *p, u32_t rem, u64_t len)
{
    u32_t n = (rem + 9u <= 64u) ? 64u : 128u;
    u64_t bits = 8u * len;

    p[rem] = 0x80;
    memset(p + rem + 1, 0, n - rem - 1);
    for (int k = 0; k < 8; k++)
        p[n - 1 - k] = (u08_t)(bits >> (8 * k));
    return n;
}

void sha1_buffer(const u08_t *data, u64_t len, u32_t h[5])
{
    u08_t last[128];
    u32_t w[16];
    u64_t full = len / 64u * 64u;

    sha1_init(h);
    for (u64_t i = 0; i < full; i += 64) {
        pack_block(data + i, w);
        sha1_update(w, h);
    }
    memcpy(last, data + full, (size_t)(len - full));
    u32_t n = sha1_pad(last, (u32_t)(len - full), len);
    for (u32_t i = 0; i < n; i += 64) {
        pack_block(last + i, w);
        sha1_update(w, h);
    }
}

void sha1_candidate(const search_template_t *t, const u32_t *w, u32_t h[5])
{
    u32_t blk[16];

    for (int i = 0; i < 5; i++)
        h[i] = t->midstate[i];
    for (u32_t b = 0; b < t->n_blocks; b++) {
        memcpy(blk, w + 16 * b, sizeof(blk)); // sha1_update takes a non-const pointer
        sha1_update(blk, h);
    }
}

//
// layouts
//

static int read_file(const char *path, u08_t **buf, u64_t *len)
{
    FILE *fp = fopen(path, "rb");
    long n;

    *buf = NULL;
    if (fp == NULL || fseek(fp, 0, SEEK_END) != 0 || (n = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fprintf(stderr, "read_file(\"%s\"): unable to read\n", path);
        if (fp != NULL)
            fclose(fp);
        return -1;
    }
    *buf = malloc((size_t)n + 1);
    if (*buf == NULL || fread(*buf, 1, (size_t)n, fp) != (size_t)n) {
        fprintf(stderr, "read_file(\"%s\"): read error\n", path);
        fclose(fp);
        free(*buf);
        *buf = NULL;
        return -1;
    }
    fclose(fp);
    *len = (u64_t)n;
    return 0;
}

// fill length for a = bytes before the fill, s = |S| (see the comment at the top); -1 if impossible
static int choose_fill(u64_t a, u64_t s)
{
    if (s <= 44u)
        return (int)((55u + 64u - (a + AAD_TEXT_VAR_LEN + s) % 64u) % 64u);
    if (s <= 55u)
        return (int)((64u - (a + AAD_TEXT_VAR_LEN) % 64u) % 64u);
    return -1;
}

// in[0..ins) + pre + fill + var + post + in[ins..]; the fill and the variable bytes (counter 0) are '?'
static int insert_text(const u08_t *in, u64_t in_len, u64_t ins, const u08_t *pre, u32_t pre_len,
                       const u08_t *post, u32_t post_len, int fill, layout_t *lo)
{
    u64_t len = in_len + pre_len + (u64_t)fill + AAD_TEXT_VAR_LEN + post_len;
    u08_t *p = malloc((size_t)len);

    if (p == NULL)
        return -1;
    lo->buf = p;
    lo->len = len;
    memcpy(p, in, (size_t)ins);                                p += ins;
    memcpy(p, pre, pre_len);                                   p += pre_len;
    memset(p, '?', (size_t)fill);                              p += fill;
    lo->var_pos = (u64_t)(p - lo->buf);
    memset(p, '?', AAD_TEXT_VAR_LEN);                          p += AAD_TEXT_VAR_LEN;
    memcpy(p, post, post_len);                                 p += post_len;
    memcpy(p, in + ins, (size_t)(in_len - ins));
    return 0;
}

static int layout_pdf(const u08_t *in, u64_t len, layout_t *lo)
{
    static const char key[] = "startxref";
    const u64_t k = sizeof(key) - 1;
    u64_t ins = 0;
    int found = 0;

    for (u64_t i = len >= k ? len - k + 1 : 0; i-- > 0;)
        if (memcmp(in + i, key, k) == 0) { ins = i; found = 1; break; }
    if (!found) {
        fprintf(stderr, "make_template: no startxref in the PDF file\n");
        return -1;
    }
    // the comment must start a line; "\r" or "\r\n" before startxref also end a line
    const char *pre = (ins > 0 && (in[ins - 1] == '\n' || in[ins - 1] == '\r')) ? "%" : "\n%";
    u32_t pre_len = (u32_t)strlen(pre);
    int fill = choose_fill(ins + pre_len, 1u + (len - ins));
    if (fill < 0) {
        fprintf(stderr, "make_template: too much data after startxref\n");
        return -1;
    }
    return insert_text(in, len, ins, (const u08_t *)pre, pre_len, (const u08_t *)"\n", 1, fill, lo);
}

static u32_t get_be32(const u08_t *p)
{
    return (u32_t)p[0] << 24 | (u32_t)p[1] << 16 | (u32_t)p[2] << 8 | (u32_t)p[3];
}

static void put_be32(u08_t *p, u32_t v)
{
    p[0] = (u08_t)(v >> 24); p[1] = (u08_t)(v >> 16); p[2] = (u08_t)(v >> 8); p[3] = (u08_t)v;
}

static int layout_png(const u08_t *in, u64_t len, layout_t *lo)
{
    static const u08_t sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    static const char head[] = "tEXtComment"; // chunk type + keyword; a NUL follows the keyword
    u08_t pre[16], post[4] = { 0, 0, 0, 0 };
    u64_t off = 8, ins = 0;
    int found = 0;

    if (len < 8 || memcmp(in, sig, 8) != 0) {
        fprintf(stderr, "make_template: bad PNG signature\n");
        return -1;
    }
    while (off + 12u <= len) {
        u64_t data_len = get_be32(in + off);
        if (memcmp(in + off + 4, "IEND", 4) == 0) { ins = off; found = 1; break; }
        off += 12u + data_len;
    }
    if (!found) {
        fprintf(stderr, "make_template: no IEND chunk in the PNG file\n");
        return -1;
    }
    int fill = choose_fill(ins + sizeof(pre), sizeof(post) + (len - ins));
    if (fill < 0) {
        fprintf(stderr, "make_template: too much data after IEND\n");
        return -1;
    }
    put_be32(pre, (u32_t)(8 + fill + AAD_TEXT_VAR_LEN)); // "Comment" + NUL + fill + variable bytes
    memcpy(pre + 4, head, 11);
    pre[15] = 0;
    if (insert_text(in, len, ins, pre, sizeof(pre), post, sizeof(post), fill, lo) != 0)
        return -1;
    // CRC over chunk type + data: register before the variable bytes, and the CRC for counter 0
    const char *crc_start = (const char *)lo->buf + ins + 4;
    lo->crc_prefix = update_png_crc(0xFFFFFFFFu, crc_start, 12 + fill);
    lo->crc_pos = lo->var_pos + AAD_TEXT_VAR_LEN;
    put_be32(lo->buf + lo->crc_pos, compute_png_crc(crc_start, 12 + fill + AAD_TEXT_VAR_LEN));
    return 0;
}

// the teacher's reader: header "P6 W H 255" (no comments) followed by 3*W*H bytes
static int layout_ppm(const char *in_path, layout_t *lo)
{
    ppm_t ppm;

    if (read_ppm_file(in_path, &ppm) != 0 || ppm.rgb_size < AAD_PPM_VAR_LEN) {
        free(ppm.data);
        return -1;
    }
    lo->buf = ppm.data;
    lo->len = (u64_t)ppm.file_size;
    lo->var_pos = lo->len - AAD_PPM_VAR_LEN;
    for (u64_t j = lo->var_pos; j < lo->len; j++)
        lo->buf[j] &= 0xFE; // counter 0
    return 0;
}

static int layout_appended(const u08_t *in, u64_t len, file_type_t type, layout_t *lo)
{
    const char *pre, *post;

    if (type == FT_HTML) {
        pre = "<!--";
        post = "-->\n";
    } else {
        pre = (len > 0 && in[len - 1] != '\n') ? "\n" : ""; // the last line must be ours
        post = "\n";
    }
    u32_t pre_len = (u32_t)strlen(pre), post_len = (u32_t)strlen(post);
    return insert_text(in, len, len, (const u08_t *)pre, pre_len, (const u08_t *)post, post_len,
                       choose_fill(len + pre_len, post_len), lo);
}

static int build_layout(const char *in_path, file_type_t type, layout_t *lo)
{
    u08_t *in;
    u64_t len;
    int r;

    memset(lo, 0, sizeof(*lo));
    if (type == FT_PPM)
        return layout_ppm(in_path, lo);
    if (read_file(in_path, &in, &len) != 0)
        return -1;
    switch (type) {
        case FT_PDF: r = layout_pdf(in, len, lo); break;
        case FT_PNG: r = layout_png(in, len, lo); break;
        case FT_HTML:
        case FT_TXT: r = layout_appended(in, len, type, lo); break;
        default: r = -1;
    }
    free(in);
    return r;
}

int make_template(const char *in_path, file_type_t type, search_template_t *t)
{
    layout_t lo;
    u32_t w[16];

    if (build_layout(in_path, type, &lo) != 0) {
        free(lo.buf);
        return -1;
    }
    u64_t padded = (lo.len + 9u + 63u) / 64u * 64u;
    u64_t prefix = lo.var_pos / 64u * 64u; // the layouts above keep padded - prefix <= 128
    memset(t, 0, sizeof(*t));
    t->type = type;
    t->n_blocks = (u32_t)((padded - prefix) / 64u);
    t->var_off = (u32_t)(lo.var_pos - prefix);
    t->var_len = (type == FT_PPM) ? AAD_PPM_VAR_LEN : AAD_TEXT_VAR_LEN;
    t->crc_off = (type == FT_PNG) ? (u32_t)(lo.crc_pos - prefix) : 0u;
    t->crc_prefix = (type == FT_PNG) ? lo.crc_prefix : 0u;
    t->h4_mask = 0xFFFFFFFFu;
    t->prefix_len = prefix;
    t->total_len = lo.len;
    memcpy(t->tail, lo.buf + prefix, (size_t)(lo.len - prefix));
    sha1_pad(t->tail, (u32_t)(lo.len - prefix), lo.len);
    sha1_init(t->midstate);
    for (u64_t i = 0; i < prefix; i += 64) {
        pack_block(lo.buf + i, w);
        sha1_update(w, t->midstate);
    }
    free(lo.buf);
    return 0;
}

int write_result(const char *in_path, const search_template_t *t, u64_t counter, const char *out_path)
{
    layout_t lo;
    u08_t b[128];
    FILE *fp;
    int r = -1;

    if (build_layout(in_path, t->type, &lo) != 0)
        goto done;
    u64_t n = t->total_len - t->prefix_len;
    if (lo.len != t->total_len || memcmp(lo.buf + t->prefix_len, t->tail, (size_t)n) != 0) {
        fprintf(stderr, "write_result: \"%s\" does not match the search template\n", in_path);
        goto done;
    }
    memcpy(b, t->tail, sizeof(b));
    aad_apply_counter(t, counter, b);
    memcpy(lo.buf + t->prefix_len, b, (size_t)n);
    fp = fopen(out_path, "wb");
    if (fp == NULL || fwrite(lo.buf, 1, (size_t)lo.len, fp) != (size_t)lo.len) {
        fprintf(stderr, "write_result: unable to write \"%s\"\n", out_path);
        if (fp != NULL)
            fclose(fp);
        goto done;
    }
    r = (fclose(fp) == 0) ? 0 : -1;
done:
    free(lo.buf);
    return r;
}
