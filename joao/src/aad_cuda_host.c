//
// aad_cuda_host.c --- STUB (owner: António): CUDA engine (host side, driver API)
//
// Placeholder so that the project links; it calls search_scalar until the real engine exists.
//

#include "aad_engines.h"

int cuda_init(const search_template_t *t)
{
    (void)t;
    return 0;
}

u64_t search_cuda(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
    return search_scalar(t, first, count, found, max_found, n_found);
}

void cuda_done(void)
{
}
