//
// aad_avx2.c --- STUB (owner: António): avx2 engine, one core
//
// Placeholder so that the project links; it calls search_scalar until the real engine exists.
//

#include "aad_engines.h"

u64_t search_avx2(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
    return search_scalar(t, first, count, found, max_found, n_found);
}
