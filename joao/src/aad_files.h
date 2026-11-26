//
// aad_files.h --- file formats (PDF, PNG, PPM, HTML, TXT), search template and SHA1 helpers
//

#ifndef AAD_FILES_H
#define AAD_FILES_H

#include "aad_search.h"

// file type from the extension (.pdf .png .ppm .html/.htm .txt); 0 = ok, -1 = unknown
int file_type_from_path(const char *path, file_type_t *type);
const char *file_type_name(file_type_t type);

// builds the search template of a file (midstate, tail, offsets); 0 = ok
int make_template(const char *in_path, file_type_t type, search_template_t *t);

// writes the modified file for the given counter; 0 = ok
int write_result(const char *in_path, const search_template_t *t, u64_t counter, const char *out_path);

// full SHA1 of a buffer (padding included), as 5 state words H0..H4
void sha1_buffer(const u08_t *data, u64_t len, u32_t h[5]);

// SHA1 of a candidate: midstate followed by the n_blocks tail blocks in w[] (as build_candidate)
void sha1_candidate(const search_template_t *t, const u32_t *w, u32_t h[5]);

#endif
