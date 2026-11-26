//
// aad_mutate.h --- the single definition of "counter -> candidate" (C and CUDA)
//
//   build_candidate(t, counter, w) fills w[0 .. 16*n_blocks-1] with the tail blocks of the
//   candidate file for that counter, packed as the SHA1 macro expects (big-endian words).
//
// Engines may specialise (recompute only the words that change), but their result must be
// identical to this function. The PNG CRC is computed bit by bit so that no table is needed
// on the device.
//

#ifndef AAD_MUTATE_H
#define AAD_MUTATE_H

#include "aad_search.h"

#if defined(__CUDACC__)
# define AAD_HD __host__ __device__ static inline
#else
# define AAD_HD static inline
#endif

// text types: variable byte j (j = 0 is the first in the file) is a character in '?'..'~';
// the character that changes fastest is the last one
AAD_HD u08_t aad_text_char(u64_t counter, u32_t j)
{
    return (u08_t)(0x3Fu + (u32_t)((counter >> (6u * (10u - j))) & 63u));
}

// PNG CRC32 continued over one byte (reflected polynomial 0xEDB88320, same as aad_png_crc.h)
AAD_HD u32_t aad_crc32_byte(u32_t crc, u08_t byte)
{
    crc ^= byte;
    for (int k = 0; k < 8; k++)
        crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    return crc;
}

// writes the variable bytes (and the PNG CRC) of a candidate into a copy of the tail
AAD_HD void aad_apply_counter(const search_template_t *t, u64_t counter, u08_t *b)
{
    if (t->type == FT_PPM) {
        for (u32_t j = 0; j < AAD_PPM_VAR_LEN; j++)
            b[t->var_off + j] = (u08_t)((b[t->var_off + j] & 0xFEu) | ((counter >> (47u - j)) & 1u));
        return;
    }
    for (u32_t j = 0; j < AAD_TEXT_VAR_LEN; j++)
        b[t->var_off + j] = aad_text_char(counter, j);
    if (t->type == FT_PNG) {
        u32_t crc = t->crc_prefix;
        for (u32_t j = 0; j < AAD_TEXT_VAR_LEN; j++)
            crc = aad_crc32_byte(crc, b[t->var_off + j]);
        crc ^= 0xFFFFFFFFu;
        b[t->crc_off + 0] = (u08_t)(crc >> 24);
        b[t->crc_off + 1] = (u08_t)(crc >> 16);
        b[t->crc_off + 2] = (u08_t)(crc >> 8);
        b[t->crc_off + 3] = (u08_t)crc;
    }
}

AAD_HD void build_candidate(const search_template_t *t, u64_t counter, u32_t w[32])
{
    u08_t b[128];
    u32_t n = 64u * t->n_blocks;

    for (u32_t i = 0; i < n; i++)
        b[i] = t->tail[i];
    aad_apply_counter(t, counter, b);
    for (u32_t i = 0; i < n / 4u; i++)
        w[i] = (u32_t)b[4 * i] << 24 | (u32_t)b[4 * i + 1] << 16 | (u32_t)b[4 * i + 2] << 8 | (u32_t)b[4 * i + 3];
}

#endif
