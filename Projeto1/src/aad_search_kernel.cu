
// aad_search_kernel.cu --- kernel CUDA de procura. Autor: António


#include "aad_sha1.h"         // a macro SHA1_UPDATE
#include "aad_search.h"       // search_template_t
#include "aad_mutate.h"       // build_candidate (funciona também na GPU)

extern "C" __global__ __launch_bounds__(RECOMENDED_CUDA_BLOCK_SIZE,1)
void procura_kernel(const search_template_t *t, u64_t base, u32_t quantos, u32_t *storage)
{
  u32_t n, idx, i;
  u64_t counter;
  u32_t data[32], hash[5];          // cada thread tem os seus (privados); 32 = 1 ou 2 blocos

  n = (u32_t)threadIdx.x + (u32_t)blockDim.x * (u32_t)blockIdx.x;   // nº global da thread
  if(n >= quantos)                  // P6: programação defensiva (threads a mais no último bloco)
    return;
  counter = base + n;               // o contador desta thread

  build_candidate(t, counter, data);   // o fim do ficheiro com o contador lá escrito

  for(i = 0; i < 5; i++)
    hash[i] = t->midstate[i];          // parte do midstate

# define T                u32_t
# define C(c)             (c)
# define ROTATE(x,n)      (((x) << (n)) | ((x) >> (32 - (n))))
# define HASH(idx)        hash[idx]
# define CHUNK_DATA(idx)  data[idx]
  SHA1_UPDATE();                        // 1.º bloco
# undef CHUNK_DATA
  if(t->n_blocks == 2)
  {
#   define CHUNK_DATA(idx)  data[16 + (idx)]
    SHA1_UPDATE();                      // 2.º bloco (continua do hash do 1.º)
#   undef CHUNK_DATA
  }
# undef T
# undef C
# undef ROTATE
# undef HASH

  if((hash[4] & t->h4_mask) == 0)      // sucesso?
  {
    idx = atomicAdd(&storage[0], 2u);   // reserva 2 posições
    if(idx <= 1024u - 2u)                // não escrever fora da storage_area
    {
      storage[idx]     = (u32_t)counter;           // o contador vencedor (32 bits de baixo)
      storage[idx + 1] = (u32_t)(counter >> 32);   // (32 bits de cima)
    }
  }
}
