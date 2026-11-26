#include "aad_sha1.h"
typedef unsigned int u32_t;

#define MASK 0x000FFFFFu

extern "C" __global__ __launch_bounds__(RECOMENDED_CUDA_BLOCK_SIZE,1)
void procura_kernel(u32_t *mid, u32_t *bloco2, u32_t base, u32_t *storage)
{
  u32_t n, counter, idx, i;
  u32_t data[16], hash[5];          // cada thread tem os seus (privados)

  n = (u32_t)threadIdx.x + (u32_t)blockDim.x * (u32_t)blockIdx.x;   // nº global da thread
  counter = base + n;                 // o contador desta thread

  for(i = 0; i < 16; i++)
    data[i] = bloco2[i];
  data[0] = counter;                     // o contador vai na palavra 0 (como no AVX)

  for(i = 0; i < 5; i++)
    hash[i] = mid[i];                   // parte do midstate

# define T                u32_t
# define C(c)             (c)
# define ROTATE(x,n)      (((x) << (n)) | ((x) >> (32 - (n))))
# define CHUNK_DATA(idx)  data[idx]
# define HASH(idx)        hash[idx]
  SHA1_UPDATE();
# undef T
# undef C
# undef ROTATE
# undef CHUNK_DATA
# undef HASH

  if((hash[4] & MASK) == 0)        // sucesso?
  {
    idx = atomicAdd(&storage[0], 2u);   // reserva 2 posições
    if(idx <= 1024u - 2u)                // não escrever fora da storage_area
    {
      storage[idx]     = counter;        // o contador vencedor
      storage[idx + 1] = hash[4];        // e o H4, para confirmar no CPU
    }
  }
}