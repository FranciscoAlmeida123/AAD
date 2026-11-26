//
// aad_search.h --- shared types of the SHA1 search (file type and search template)
//
// A search template holds everything an engine needs: the SHA1 midstate of the fixed prefix
// blocks and the final 1 or 2 blocks (tail) that change with each candidate.
//

#ifndef AAD_SEARCH_H
#define AAD_SEARCH_H

#include "aad_data_types.h"

typedef enum { FT_PDF = 0, FT_PNG = 1, FT_PPM = 2, FT_HTML = 3, FT_TXT = 4 } file_type_t;

typedef struct {
    file_type_t type;
    u32_t midstate[5];     // SHA1 state after the fixed prefix blocks
    u32_t n_blocks;        // 1 or 2 final 64-byte blocks recomputed per attempt
    u08_t tail[128];       // those final blocks, SHA1 padding included (variable bytes as for counter 0)
    u32_t var_off;         // offset in tail of the first variable byte
    u32_t var_len;         // 11 for text types; 48 (bytes whose LSB varies) for PPM
    u32_t crc_off;         // PNG only: offset in tail of the tEXt CRC
    u32_t crc_prefix;      // PNG only: CRC32 register before the final XOR, after "tEXt"+keyword+NUL+fill
    u32_t h4_mask;         // success if (H4 & h4_mask) == 0; 0xFFFFFFFF for the real goal
    u64_t prefix_len;      // file bytes before tail (to rebuild the output file)
    u64_t total_len;       // length of the final file (where tail data ends and SHA1 padding starts)
} search_template_t;

#define AAD_TEXT_VAR_LEN 11  // variable characters in text types (6 bits each)
#define AAD_PPM_VAR_LEN  48  // colour bytes whose LSB varies in PPM (1 bit each)

#endif
