//
// aad_engines.h --- declarations of the search engines
//
// Each engine tests the counters [first, first+count) and returns the number of SHA1 block
// updates done. Up to max_found winning counters are stored in found[]; *n_found is the total
// number found (it may exceed max_found).
//
// search_scalar is in aad_scalar.c; the others are in aad_avx.c, aad_avx2.c, aad_avx512.c,
// aad_omp.c and aad_cuda_host.c.
//

#ifndef AAD_ENGINES_H
#define AAD_ENGINES_H

#include "aad_search.h"

u64_t search_scalar(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found);
u64_t search_avx   (const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found);
u64_t search_avx2  (const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found);
u64_t search_avx512(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found);
u64_t search_omp   (const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found,
                    int n_threads);
int   cuda_init(const search_template_t *t);   // device, module, memory: outside the measured time
u64_t search_cuda  (const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found);
void  cuda_done(void);

#endif
