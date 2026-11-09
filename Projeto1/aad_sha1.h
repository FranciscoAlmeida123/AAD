//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// template for the computation of the SHA1 secure hash (processes ONLY one chunk of data)
//


//
// partial computation of the SHA1 secure hash (one chunk)
//
// the general SHA1 secure hash algorithm ingests data in chunks of 64 bytes; at the end there must
// be one byte of padding, with value 0x80, zero or more bytes of zeros, also for padding, appended
// until the last chunk has 56 bytes, and a final 8 byte integer holding the number of bits of the
// entire message
//

#ifndef AAD_SHA1
#define AAD_SHA1


//
// number of threads in each CUDA block
//
// we place this here to simplify things (aad_sha1_cuda_kernel.cu includes this file...)
//
#ifndef RECOMENDED_CUDA_BLOCK_SIZE
# define RECOMENDED_CUDA_BLOCK_SIZE  128
#endif


//
// each chunk has exactly 64 bytes, which must be stored in a 32-bit array with 16 elements
// (see https://datatracker.ietf.org/doc/html/rfc3174 or the file rfc3174.txt for more details)
//
// each chunk is processed using a macro called SHA1_UPDATE
// it must be customized using the following additional macros:
//   T               --- the data type
//   C(c)            --- how to expand the constant c
//   ROTATE(x,n)     --- how to rotate x left by n bits
//   CHUNK_DATA(idx) --- how to access the data at index idx, 0 <= idx <= 15
//   HASH(idx)       --- how to access the hash at index idx, 0 <= idx <= 4
// see aad_sha1_cpu.h for examples
//
// each chunk is stored in the locations
//   CHUNK_DATA(0), CHUNK_DATA(1), ..., CHUNK_DATA(15)
// the updated state of the SHA1 secure hash is stored in the locations
//   HASH(0), HASH(1), ..., HASH(4)
//

//
// first group of 20 iterations (0 <= t <= 19)
//
#define SHA1_F1(x,y,z)  ((x & y) | (~x & z))
#define SHA1_K1         0x5A827999u

//
// second group of 20 iterations (20 <= t <= 39)
//
#define SHA1_F2(x,y,z)  (x ^ y ^ z)
#define SHA1_K2         0x6ED9EBA1u

//
// third group of 20 iterations (40 <= t <= 59)
//
#define SHA1_F3(x,y,z)  ((x & y) | (x & z) | (y & z))
#define SHA1_K3         0x8F1BBCDCu

//
// fourth group of 20 iterations (60 <= t <= 79)
//
#define SHA1_F4(x,y,z)  (x ^ y ^ z)
#define SHA1_K4         0xCA62C1D6u

//
// data mixing function
//
#define SHA1_D(t)                                                                            \
  do                                                                                         \
  {                                                                                          \
    T tmp = w[((t) - 3) & 15] ^ w[((t) - 8) & 15] ^ w[((t) - 14) & 15] ^ w[((t) - 16) & 15]; \
    w[(t) & 15] = ROTATE(tmp,1);                                                             \
  }                                                                                          \
  while(0)

//
// state mixing function
//
#define SHA1_S(F,t,K)                                                                        \
  do                                                                                         \
  {                                                                                          \
    T tmp = ROTATE(a,5) + F(b,c,d) + e + w[(t) & 15] + C(K);                                 \
    e = d;                                                                                   \
    d = c;                                                                                   \
    c = ROTATE(b,30);                                                                        \
    b = a;                                                                                   \
    a = tmp;                                                                                 \
  }                                                                                          \
  while(0)

//
// initial hash values, H(0) = C(initial_H0), H(1) = C(initial_H1), ...
//
#define initial_H0 0x67452301u
#define initial_H1 0xEFCDAB89u
#define initial_H2 0x98BADCFEu
#define initial_H3 0x10325476u
#define initial_H4 0xC3D2E1F0u

//
// the SHA1_UPDATE macro, for a little-endian processor
//
// everything is loop unrolled to make sure all indices are static integers, so the compiler
// has no excuse to produce sub-optimal code (the w[16] array can even become 16 separate
// integer variables, the CUDA compiler actually does this)
//
#define SHA1_UPDATE()                                                                       \
  do                                                                                        \
  {                                                                                         \
    /* local variables */                                                                   \
    T a,b,c,d,e,w[16];                                                                      \
    /* initial state */                                                                     \
    a = HASH(0);                                                                            \
    b = HASH(1);                                                                            \
    c = HASH(2);                                                                            \
    d = HASH(3);                                                                            \
    e = HASH(4);                                                                            \
    /* copy data to the internal buffer */                                                  \
    w[ 0] = CHUNK_DATA( 0);                                                                 \
    w[ 1] = CHUNK_DATA( 1);                                                                 \
    w[ 2] = CHUNK_DATA( 2);                                                                 \
    w[ 3] = CHUNK_DATA( 3);                                                                 \
    w[ 4] = CHUNK_DATA( 4);                                                                 \
    w[ 5] = CHUNK_DATA( 5);                                                                 \
    w[ 6] = CHUNK_DATA( 6);                                                                 \
    w[ 7] = CHUNK_DATA( 7);                                                                 \
    w[ 8] = CHUNK_DATA( 8);                                                                 \
    w[ 9] = CHUNK_DATA( 9);                                                                 \
    w[10] = CHUNK_DATA(10);                                                                 \
    w[11] = CHUNK_DATA(11);                                                                 \
    w[12] = CHUNK_DATA(12);                                                                 \
    w[13] = CHUNK_DATA(13);                                                                 \
    w[14] = CHUNK_DATA(14);                                                                 \
    w[15] = CHUNK_DATA(15);                                                                 \
    /* first group of 20 iterations (0 <= t <= 19) */                                       \
                SHA1_S(SHA1_F1, 0,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 1,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 2,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 3,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 4,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 5,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 6,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 7,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 8,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1, 9,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1,10,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1,11,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1,12,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1,13,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1,14,SHA1_K1);                                                 \
                SHA1_S(SHA1_F1,15,SHA1_K1);                                                 \
    SHA1_D(16); SHA1_S(SHA1_F1,16,SHA1_K1);                                                 \
    SHA1_D(17); SHA1_S(SHA1_F1,17,SHA1_K1);                                                 \
    SHA1_D(18); SHA1_S(SHA1_F1,18,SHA1_K1);                                                 \
    SHA1_D(19); SHA1_S(SHA1_F1,19,SHA1_K1);                                                 \
    /* second group of 20 iterations (20 <= t <= 39) */                                     \
    SHA1_D(20); SHA1_S(SHA1_F2,20,SHA1_K2);                                                 \
    SHA1_D(21); SHA1_S(SHA1_F2,21,SHA1_K2);                                                 \
    SHA1_D(22); SHA1_S(SHA1_F2,22,SHA1_K2);                                                 \
    SHA1_D(23); SHA1_S(SHA1_F2,23,SHA1_K2);                                                 \
    SHA1_D(24); SHA1_S(SHA1_F2,24,SHA1_K2);                                                 \
    SHA1_D(25); SHA1_S(SHA1_F2,25,SHA1_K2);                                                 \
    SHA1_D(26); SHA1_S(SHA1_F2,26,SHA1_K2);                                                 \
    SHA1_D(27); SHA1_S(SHA1_F2,27,SHA1_K2);                                                 \
    SHA1_D(28); SHA1_S(SHA1_F2,28,SHA1_K2);                                                 \
    SHA1_D(29); SHA1_S(SHA1_F2,29,SHA1_K2);                                                 \
    SHA1_D(30); SHA1_S(SHA1_F2,30,SHA1_K2);                                                 \
    SHA1_D(31); SHA1_S(SHA1_F2,31,SHA1_K2);                                                 \
    SHA1_D(32); SHA1_S(SHA1_F2,32,SHA1_K2);                                                 \
    SHA1_D(33); SHA1_S(SHA1_F2,33,SHA1_K2);                                                 \
    SHA1_D(34); SHA1_S(SHA1_F2,34,SHA1_K2);                                                 \
    SHA1_D(35); SHA1_S(SHA1_F2,35,SHA1_K2);                                                 \
    SHA1_D(36); SHA1_S(SHA1_F2,36,SHA1_K2);                                                 \
    SHA1_D(37); SHA1_S(SHA1_F2,37,SHA1_K2);                                                 \
    SHA1_D(38); SHA1_S(SHA1_F2,38,SHA1_K2);                                                 \
    SHA1_D(39); SHA1_S(SHA1_F2,39,SHA1_K2);                                                 \
    /* third group of 20 iterations (40 <= t <= 59) */                                      \
    SHA1_D(40); SHA1_S(SHA1_F3,40,SHA1_K3);                                                 \
    SHA1_D(41); SHA1_S(SHA1_F3,41,SHA1_K3);                                                 \
    SHA1_D(42); SHA1_S(SHA1_F3,42,SHA1_K3);                                                 \
    SHA1_D(43); SHA1_S(SHA1_F3,43,SHA1_K3);                                                 \
    SHA1_D(44); SHA1_S(SHA1_F3,44,SHA1_K3);                                                 \
    SHA1_D(45); SHA1_S(SHA1_F3,45,SHA1_K3);                                                 \
    SHA1_D(46); SHA1_S(SHA1_F3,46,SHA1_K3);                                                 \
    SHA1_D(47); SHA1_S(SHA1_F3,47,SHA1_K3);                                                 \
    SHA1_D(48); SHA1_S(SHA1_F3,48,SHA1_K3);                                                 \
    SHA1_D(49); SHA1_S(SHA1_F3,49,SHA1_K3);                                                 \
    SHA1_D(50); SHA1_S(SHA1_F3,50,SHA1_K3);                                                 \
    SHA1_D(51); SHA1_S(SHA1_F3,51,SHA1_K3);                                                 \
    SHA1_D(52); SHA1_S(SHA1_F3,52,SHA1_K3);                                                 \
    SHA1_D(53); SHA1_S(SHA1_F3,53,SHA1_K3);                                                 \
    SHA1_D(54); SHA1_S(SHA1_F3,54,SHA1_K3);                                                 \
    SHA1_D(55); SHA1_S(SHA1_F3,55,SHA1_K3);                                                 \
    SHA1_D(56); SHA1_S(SHA1_F3,56,SHA1_K3);                                                 \
    SHA1_D(57); SHA1_S(SHA1_F3,57,SHA1_K3);                                                 \
    SHA1_D(58); SHA1_S(SHA1_F3,58,SHA1_K3);                                                 \
    SHA1_D(59); SHA1_S(SHA1_F3,59,SHA1_K3);                                                 \
    /* fourth group of 20 iterations (60 <= t <= 79) */                                     \
    SHA1_D(60); SHA1_S(SHA1_F4,60,SHA1_K4);                                                 \
    SHA1_D(61); SHA1_S(SHA1_F4,61,SHA1_K4);                                                 \
    SHA1_D(62); SHA1_S(SHA1_F4,62,SHA1_K4);                                                 \
    SHA1_D(63); SHA1_S(SHA1_F4,63,SHA1_K4);                                                 \
    SHA1_D(64); SHA1_S(SHA1_F4,64,SHA1_K4);                                                 \
    SHA1_D(65); SHA1_S(SHA1_F4,65,SHA1_K4);                                                 \
    SHA1_D(66); SHA1_S(SHA1_F4,66,SHA1_K4);                                                 \
    SHA1_D(67); SHA1_S(SHA1_F4,67,SHA1_K4);                                                 \
    SHA1_D(68); SHA1_S(SHA1_F4,68,SHA1_K4);                                                 \
    SHA1_D(69); SHA1_S(SHA1_F4,69,SHA1_K4);                                                 \
    SHA1_D(70); SHA1_S(SHA1_F4,70,SHA1_K4);                                                 \
    SHA1_D(71); SHA1_S(SHA1_F4,71,SHA1_K4);                                                 \
    SHA1_D(72); SHA1_S(SHA1_F4,72,SHA1_K4);                                                 \
    SHA1_D(73); SHA1_S(SHA1_F4,73,SHA1_K4);                                                 \
    SHA1_D(74); SHA1_S(SHA1_F4,74,SHA1_K4);                                                 \
    SHA1_D(75); SHA1_S(SHA1_F4,75,SHA1_K4);                                                 \
    SHA1_D(76); SHA1_S(SHA1_F4,76,SHA1_K4);                                                 \
    SHA1_D(77); SHA1_S(SHA1_F4,77,SHA1_K4);                                                 \
    SHA1_D(78); SHA1_S(SHA1_F4,78,SHA1_K4);                                                 \
    SHA1_D(79); SHA1_S(SHA1_F4,79,SHA1_K4);                                                 \
    /* update state (in this special case, finish) */                                       \
    HASH(0) += a;                                                                           \
    HASH(1) += b;                                                                           \
    HASH(2) += c;                                                                           \
    HASH(3) += d;                                                                           \
    HASH(4) += e;                                                                           \
  }                                                                                         \
  while(0)


//
// the end!
//

#endif
