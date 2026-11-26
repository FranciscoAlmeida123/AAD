//
// aad_scalar.c --- reference search engine: one CPU core, no SIMD
//
//   search_scalar(t, first, count, found, max_found, n_found)
//
// Per counter: write the variable bytes with aad_apply_counter (the same code as build_candidate),
// repack only the words that hold them, and run the n_blocks tail blocks from the midstate.
//

#include <string.h>
#include "aad_engines.h"
#include "aad_mutate.h"
#include "aad_sha1_cpu.h"

u64_t search_scalar(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
    u08_t b[128];
    u32_t w[32], h[5];
    u32_t end = (t->type == FT_PNG) ? t->crc_off + 4u : t->var_off + t->var_len;
    u32_t lo = t->var_off / 4u, hi = (end - 1u) / 4u; // words with variable bytes (or the PNG CRC)

    memcpy(b, t->tail, sizeof(b));
    build_candidate(t, first, w);
    *n_found = 0;
    for (u64_t i = 0; i < count; i++) {
        u64_t c = first + i;
        aad_apply_counter(t, c, b);
        for (u32_t k = lo; k <= hi; k++)
            w[k] = (u32_t)b[4 * k] << 24 | (u32_t)b[4 * k + 1] << 16 | (u32_t)b[4 * k + 2] << 8 | (u32_t)b[4 * k + 3];
        for (int k = 0; k < 5; k++)
            h[k] = t->midstate[k];
        for (u32_t blk = 0; blk < t->n_blocks; blk++)
            sha1_update(&w[16 * blk], h);
        if ((h[4] & t->h4_mask) == 0u) {
            if (*n_found < max_found)
                found[*n_found] = c;
            (*n_found)++;
        }
    }
    return count * t->n_blocks;
}
