//
// cuda13_compat.h --- deixa compilar o aad_cuda_utilities.h do professor no CUDA 13 sem o alterar. Autor: António
//
// No CUDA 13 o cuCtxCreate passou a ter mais um argumento (parâmetros do contexto; NULL = por omissão).
// Usa-se com "-include tests/cuda13_compat.h" ao compilar código do professor (ex.: make sha1_cuda_test_13).
//

#include <cuda.h>
#if defined(CUDA_VERSION) && CUDA_VERSION >= 13000
# undef cuCtxCreate
# define cuCtxCreate(ctx, flags, dev) cuCtxCreate_v4(ctx, NULL, flags, dev)
#endif
