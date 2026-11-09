//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// SHA1 secure hash update implementations on the CPU (processes ONLY one chunk of data)
//

#ifndef AAD_SHA1_CPU
#define AAD_SHA1_CPU

#include "aad_sha1.h"
#define FOUR(c)  (int)(c),(int)(c),(int)(c),(int)(c)


//
// reference implementation (no SIMD instructions)
//

__attribute__((unused))
static void sha1_update(u32_t *chunk_data,u32_t *hash)
{ // one message -> one SHA1 hash
# define T                u32_t
# define C(c)             (c)
# define ROTATE(x,n)      (((x) << (n)) | ((x) >> (32 - (n))))
# define CHUNK_DATA(idx)  chunk_data[idx]
# define HASH(idx)        hash[idx]
  SHA1_UPDATE();
# undef T
# undef C
# undef ROTATE
# undef CHUNK_DATA
# undef HASH
}


//
// implementation using avx instructions (Intel/AMD)
//

#if defined(__AVX__)

__attribute__((unused))
static void sha1_update_avx(v4si *interleaved4_chunk_data,v4si *interleaved4_hash)
{ // four interleaved messages -> four interleaved SHA1 secure hashes
# define T                v4si
# define C(c)             (v4si){ FOUR(c) }
# define ROTATE(x,n)      (__builtin_ia32_pslldi128(x,n) | __builtin_ia32_psrldi128(x,32 - (n)))
# define CHUNK_DATA(idx)  interleaved4_chunk_data[idx]
# define HASH(idx)        interleaved4_hash[idx]
  SHA1_UPDATE();
# undef T
# undef C
# undef ROTATE
# undef CHUNK_DATA
# undef HASH
}

#endif


//
// implementation using avx2 instructions (Intel/AMD)
//

#if defined(__AVX2__)

__attribute__((unused))
static void sha1_update_avx2(v8si *interleaved8_chunk_data,v8si *interleaved8_hash)
{ // eight interleaved messages -> eight interleaved SHA1 secure hashes
# define T                v8si
# define C(c)             (v8si){ FOUR(c),FOUR(c) }
# define ROTATE(x,n)      (__builtin_ia32_pslldi256(x,n) | __builtin_ia32_psrldi256(x,32 - (n)))
# define CHUNK_DATA(idx)  interleaved8_chunk_data[idx]
# define HASH(idx)        interleaved8_hash[idx]
  SHA1_UPDATE();
# undef T
# undef C
# undef ROTATE
# undef CHUNK_DATA
# undef HASH
}

#endif


//
// implementation using avx512f instructions (Intel/AMD)
//

#if defined(__AVX512F__)

__attribute__((unused))
static void sha1_update_avx512f(v16si *interleaved16_chunk_data,v16si *interleaved16_hash)
{ // sixteen interleaved messages -> sixteen interleaved SHA1 secure hashes
# define T                v16si
# define C(c)             (v16si){ FOUR(c),FOUR(c),FOUR(c),FOUR(c) }
# define ROTATE(x,n)      __builtin_ia32_prold512_mask(x,n,x,0xFFFF)
# define CHUNK_DATA(idx)  interleaved16_chunk_data[idx]
# define HASH(idx)        interleaved16_hash[idx]
  SHA1_UPDATE();
# undef T
# undef C
# undef ROTATE
# undef CHUNK_DATA
# undef HASH
}

#endif


//
// implementation using neon instructions (ARM)
//

#if defined(__ARM_NEON)

__attribute__((unused))
static void sha1_update_neon(uint32x4_t *interleaved4_chunk_data,uint32x4_t *interleaved4_hash)
{ // four interleaved messages -> four interleaved SHA1 secure hashes
# define T                uint32x4_t
# define C(c)             (uint32x4_t){ FOUR(c) }
# define ROTATE(x,n)      (vshlq_n_u32(x,n) | vshrq_n_u32(x,32 - (n)))
# define CHUNK_DATA(idx)  interleaved4_chunk_data[idx]
# define HASH(idx)        interleaved4_hash[idx]
  SHA1_UPDATE();
# undef T
# undef C
# undef ROTATE
# undef CHUNK_DATA
# undef HASH
}

#endif


//
// the end!
//

#endif
