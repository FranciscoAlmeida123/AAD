// aad_cuda_host.c --- parte do CPU da CUDA (o "chefe"). Autor: António
// (como o aad_sha1_cuda_test.c do professor e o guião P6: driver API com o aad_cuda_utilities.h)
//
//   cuda_init()   liga à GPU, carrega bin/aad_search_kernel.cubin e cria as 2 áreas de memória
//   search_cuda() testa [first, first + count) na GPU
//   cuda_done()   liberta tudo
//
// Só tem CUDA a sério quando se compila com -DAAD_CUDA ("make cuda"). No "make" normal fica
// sem CUDA, para o projeto compilar em PCs sem placa NVIDIA (o meu, por exemplo).

#include <stdio.h>
#include <string.h>
#include "aad_engines.h"

#if defined(AAD_CUDA)

#include "aad_sha1.h"             // RECOMENDED_CUDA_BLOCK_SIZE
#include <cuda.h>
#if defined(CUDA_VERSION) && CUDA_VERSION >= 13000
// no CUDA 13 o cuCtxCreate tem mais um argumento (NULL = parâmetros por omissão); assim o ficheiro
// do professor, escrito para versões anteriores, compila sem ser alterado
# undef cuCtxCreate
# define cuCtxCreate(ctx, flags, dev) cuCtxCreate_v4(ctx, NULL, flags, dev)
#endif
#include "aad_cuda_utilities.h"   // do professor: initialize_cuda, host_to_device_copy, launch_kernel, ...

static cuda_data_t cd;

int cuda_init(const search_template_t *t)
{
  (void)t;
  cd.device_number = 0;                                    // primeira placa
  cd.cubin_file_name = "bin/aad_search_kernel.cubin";
  cd.kernel_name = "procura_kernel";
  cd.data_size[0] = (u32_t)sizeof(search_template_t);      // área 0: o template
  cd.data_size[1] = 1024u * (u32_t)sizeof(u32_t);          // área 1: a storage_area
  initialize_cuda(&cd);
  return 0;
}

void cuda_done(void)
{
  terminate_cuda(&cd);
}

u64_t search_cuda(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
  u32_t *storage = (u32_t *)cd.host_data[1];
  u32_t guardadas = 0, total, k, quantos;
  u64_t base, feitos;
  // cada lançamento com ~128 soluções esperadas no máximo (1 solução em 2^bits), para caber na storage_area
  int bits = __builtin_popcount(t->h4_mask);
  u64_t por_lancamento = (bits < 17) ? (128u << bits) : (1u << 24);

  // enviar o template (é pequeno, e assim serve sempre o template certo)
  memcpy(cd.host_data[0], t, sizeof(*t));
  host_to_device_copy(&cd, 0);

  *n_found = 0;
  for(feitos = 0; feitos < count; feitos += quantos)
  {
    base = first + feitos;
    quantos = (u32_t)((count - feitos < por_lancamento) ? count - feitos : por_lancamento);

    storage[0] = 1u;                          // D8: a storage_area começa em 1
    host_to_device_copy(&cd, 1);

    cd.block_dim_x = RECOMENDED_CUDA_BLOCK_SIZE;                                  // 128 threads por bloco
    cd.grid_dim_x = (quantos + RECOMENDED_CUDA_BLOCK_SIZE - 1) / RECOMENDED_CUDA_BLOCK_SIZE;  // blocos
    cd.n_kernel_arguments = 4;
    cd.arg[0] = &cd.device_data[0];           // t
    cd.arg[1] = &base;                        // base
    cd.arg[2] = &quantos;                     // quantos
    cd.arg[3] = &cd.device_data[1];           // storage
    launch_kernel(&cd);

    device_to_host_copy(&cd, 1);              // só voltam as soluções
    total = (storage[0] - 1u) / 2u;           // nº de soluções deste lançamento
    for(k = 0; k < total && k < 511u; k++)    // 511 = as que cabem nas 1024 posições
      if(guardadas < max_found)
        found[guardadas++] = (u64_t)storage[1u + 2u * k] | (u64_t)storage[2u + 2u * k] << 32;
    *n_found += total;
  }
  return count * t->n_blocks;
}

#else   // sem CUDA neste build

int cuda_init(const search_template_t *t)
{
  (void)t;
  fprintf(stderr, "cuda_init: compilado sem CUDA (usar make cuda)\n");
  return -1;
}

u64_t search_cuda(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
  return search_scalar(t, first, count, found, max_found, n_found);
}

void cuda_done(void)
{
}

#endif
