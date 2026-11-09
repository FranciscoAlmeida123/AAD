//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// test the SHA1 implementation (cpu code)
//
// execution results for a mac Mini M4 (macOS)
//       ??? core --- sha1_update()         passed (1000 tests,  19089616 chunks per second)
//                    sha1_update_neon()    passed (1000 tests,  29060644 chunks per second)
//
// execution results for a AMD EPYC 7543 32-Core Processor (slurm)
//   3691MHz core --- sha1_update()         passed (1000 tests,  13008549 chunks per second)
//                    sha1_update_avx()     passed (1000 tests,  26765680 chunks per second)
//                    sha1_update_avx2()    passed (1000 tests,  53745037 chunks per second)
//
// execution results for a Intel(R) Core(TM) Ultra 7 155U (ubuntu, performance power mode)
//   4800MHz core --- sha1_update()         passed (1000 tests,  20265700 chunks per second)
//                    sha1_update_avx()     passed (1000 tests,  34080430 chunks per second)
//                    sha1_update_avx2()    passed (1000 tests,  66172325 chunks per second)
//   3800MHz core --- sha1_update()         passed (1000 tests,  12438158 chunks per second)
//                    sha1_update_avx()     passed (1000 tests,  26405741 chunks per second)
//                    sha1_update_avx2()    passed (1000 tests,  30083572 chunks per second)
//   2100MHz core --- sha1_update()         passed (1000 tests,   6931929 chunks per second)
//                    sha1_update_avx()     passed (1000 tests,  14614161 chunks per second)
//                    sha1_update_avx2()    passed (1000 tests,  16642517 chunks per second)
//
// execution results for a AMD Ryzen 7 9700X 8-Core Processor (ubuntu, balanced power mode)
//   5500MHz core --- sha1_update()         passed (1000 tests,  21586399 chunks per second)
//                    sha1_update_avx()     passed (1000 tests,  27215356 chunks per second)
//                    sha1_update_avx2()    passed (1000 tests,  54414103 chunks per second)
//                    sha1_update_avx512f() passed (1000 tests, 108610870 chunks per second)
//
// execution results for a Intel(R) Xeon(R) 6731P (slurm)
//   3718MHz core --- sha1_update()         passed (1000 tests,  17657366 chunks per second)
//                    sha1_update_avx()     passed (1000 tests,  40221367 chunks per second)
//                    sha1_update_avx2()    passed (1000 tests,  77677244 chunks per second)
//                    sha1_update_avx512f() passed (1000 tests, 149412644 chunks per second)
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "aad_data_types.h"
#include "aad_utilities.h"
#include "aad_sha1_cpu.h"


//
// test the reference CPU implementation (define USE_TMP_FILE to inspect the test file contents if something is wrong)
//

#undef TMP_FILE_NAME
#if defined(USE_TMP_FILE) || defined(__APPLE__)
# define TMP_FILE_NAME "/tmp/sha1_test"
#endif

static void test_sha1_update(int n_tests,int n_measurements)
{
  u08_t message_with_padding[256 + 64];               // message data with padding
  union { u08_t c[16 * 4]; u32_t i[16]; } chunk_data; // the chunk data as bytes and as 32-bit integers
  union { u08_t c[ 5 * 4]; u32_t i[ 5]; } hash;       // the hash as bytes and as 32-bit integers
#ifdef TMP_FILE_NAME
  char command[64];   //   64 is more than enough if TMP_FILE_NAME expands to a small string
#else
  char command[1056]; // 1056 is more than enough, MUST be at least max(n_bytes)*4+27
#endif
  char response[48];  //   48 is more than enough
  char computed[48];  //   48 is more than enough, MUST be at least 41
  int n,i,j,idx,n_bytes,t_bytes;
  double chunks_per_second;
  u64_t n_bits;
  u32_t sum;
  FILE *fp;

  // test
  response[40] = '\0'; // 20 bytes -> 40 hexadecimal digits
  for(n = 0;n < n_tests;n++)
  {
    n_bytes = 1 + (int)random_byte(); // maximum of 256
    // create random data
    for(i = 0;i < n_bytes;i++)
      message_with_padding[i] = random_byte();
    n_bits = 8ul * (u64_t)n_bytes;
    message_with_padding[i++] = 0x80; // padding of one 1 bit followed by seven 0 bits
    if(i % 64 > 56) // less than 8 bytes left, will need another chunk
      while(i % 64 != 0)
        message_with_padding[i++] = 0x00; // padding of eight zeros
    while(i % 64 < 56)
      message_with_padding[i++] = 0x00; // padding of eight zeros
    for(j = 56;i % 64 != 0;j -= 8)
      message_with_padding[i++] = (u08_t)(n_bits >> j); // append the number of bits of the message
    t_bytes = i;
    // initialize the hash data
    hash.i[0] = initial_H0;
    hash.i[1] = initial_H1;
    hash.i[2] = initial_H2;
    hash.i[3] = initial_H3;
    hash.i[4] = initial_H4;
    // do complete chunks
    for(i = 0;i < t_bytes;i += 64)
    {
      for(j = 0;j < 64;j++)
        chunk_data.c[j ^ 3] = message_with_padding[i + j]; // the ^ 3 is to deal with the order of bytes in an integer in a little endian architecture
      sha1_update(&chunk_data.i[0],&hash.i[0]);
    }
    // convert the secure hash into a string
    idx = 0;
    for(i = 0;i < 20;i++)
      idx += sprintf(&computed[idx],"%02x",(int)hash.c[i ^ 3] & 0xFF);
#ifdef TMP_FILE_NAME
    // create the test file and construct the command to test the SHA1 secure hash
    if((fp = fopen(TMP_FILE_NAME,"w")) == NULL || fwrite((const void *)message_with_padding,(size_t)1,(size_t)n_bytes,fp) != (size_t)n_bytes || fclose(fp) != 0)
    {
      fprintf(stderr,"test_sha1_update: unable to create file %s\n",TMP_FILE_NAME);
      exit(1);
    }
    idx = sprintf(&command[0],"sha1sum %s",TMP_FILE_NAME);
#else
    // construct the command to test the SHA1 secure hash
    idx = sprintf(&command[0],"/bin/echo -en '"); // do not rely on the bash echo builtin command (the /bin/echo macOS binary does NOT handle -e) --- 15 bytes
    for(i = 0;i < n_bytes;i++)
      idx += sprintf(&command[idx],"\\x%02x",(int)message_with_padding[i] & 0xFF); // 4 more bytes
    idx += sprintf(&command[idx],"' | sha1sum"); // 11+1 more bytes
#endif
    if(idx >= (int)sizeof(command))
    { // pos-memory corruption, the test should have been done earlier, but we don't care
      fprintf(stderr,"test_sha1_update: command[] overflow\n");
      exit(1);
    }
    // run it and get its output
    fp = popen(command,"r");
    if(fp == NULL)
    {
      fprintf(stderr,"test_sha1_update: popen() failed\n");
      exit(1);
    }
    if(fread((void *)&response[0],sizeof(char),(size_t)40,fp) != (size_t)40)
    {
      fprintf(stderr,"test_sha1_update: fread() failed\n");
      exit(1);
    }
    pclose(fp);
    // compare them
    if(memcmp((void *)response,(void *)computed,(size_t)40) != 0)
    { // print everything
      fprintf(stderr,"test_sha1_update: sha1_update() failure for n=%d with n_bytes=%d:\n",n,n_bytes);
      for(i = 0;i < t_bytes;i++)
        fprintf(stderr,"  message[%3d] = %02x%s%s\n",i,(int)message_with_padding[i] & 0xFF,(i < n_bytes) ? "" : " padding",(i % 64 == 63) ? "\n" : "");
      for(i = 0;i < 20;i++)
        fprintf(stderr,"  hash[%3d] = %02x\n",i,(int)hash.c[i ^ 3] % 0xFF);
      fprintf(stderr,"  sha1sum output: %s\n",response);
      fprintf(stderr,"  sha1() output:  %s\n",computed);
      for(i = 0;i < 40 && response[i] == computed[i];i++)
        ;
      fprintf(stderr,"  mismatch at %d\n",i);
      exit(1);
    }
  }
#ifdef TMP_FILE_NAME
  (void)unlink(TMP_FILE_NAME);
#endif
  // warmup (activate turbo boost...)
  for(i = 0;i < 1000000;i++)
    (void)random_byte();
  // create random chunk
  for(i = 0;i < 64;i++)
    chunk_data.c[i] = random_byte();
  // measure
  time_measurement();
  sum = 0u;
  for(n = 0;n < n_measurements;n++)
  {
    sha1_update(&chunk_data.i[0],&hash.i[0]);
    sum += hash.i[4];
    chunk_data.i[0]++;
  }
  time_measurement();
  if(sum == 0u)
    fprintf(stderr,"test_sha1_update: what a coincidence, sum=0\n");
  chunks_per_second = (double)n_measurements / cpu_time_delta();
  // report
  printf("sha1_update() passed (%d test%s, %.0f chunks per second)\n",n_tests,(n_tests == 1) ? "" : "s",chunks_per_second);
}

#undef TMP_FILE_NAME


//
// function templete for accelerated tests (C does not have yet function templates, so we have to resort to a macro...)
//

#define DO_TEST(EXTENSION,ALIGNMENT,N_LANES,DATA_TYPE)                                                                                               \
static void test_sha1_update_ ## EXTENSION(int n_tests,int n_measurements)                                                                           \
{                                                                                                                                                    \
  static union { u08_t c[16 * 4]; u32_t i[16]; } chunk_data[N_LANES]; /* the chunk data as bytes and as 32-bit integers */                           \
  static union { u08_t c[ 5 * 4]; u32_t i[ 5]; } hash[N_LANES];       /* the hash as bytes and as 32-bit integers       */                           \
  static u32_t interleaved_chunk_data[16][N_LANES] __attribute__((aligned(ALIGNMENT)));                                                              \
  static u32_t interleaved_hash[5][N_LANES]  __attribute__((aligned(ALIGNMENT)));                                                                    \
  double chunks_per_second;                                                                                                                          \
  int n,i,lane;                                                                                                                                      \
  u32_t sum;                                                                                                                                         \
  /* test */                                                                                                                                         \
  for(n = 0;n < n_tests;n++)                                                                                                                         \
  {                                                                                                                                                  \
    /* the data and the secure hash for the reference implementation */                                                                              \
    for(lane = 0;lane < N_LANES;lane++)                                                                                                              \
    {                                                                                                                                                \
      /* create random chunk data (64 bytes) */                                                                                                      \
      for(i = 0;i < 64;i++)                                                                                                                          \
        chunk_data[lane].c[i] = random_byte();                                                                                                       \
      /* do SHA_UPDATE */                                                                                                                            \
      hash[lane].i[0] = initial_H0;                                                                                                                  \
      hash[lane].i[1] = initial_H1;                                                                                                                  \
      hash[lane].i[2] = initial_H2;                                                                                                                  \
      hash[lane].i[3] = initial_H3;                                                                                                                  \
      hash[lane].i[4] = initial_H4;                                                                                                                  \
      sha1_update(&chunk_data[lane].i[0],&hash[lane].i[0]);                                                                                          \
    }                                                                                                                                                \
    /* interleave (transpose) the data */                                                                                                            \
    for(lane = 0;lane < N_LANES;lane++)                                                                                                              \
    {                                                                                                                                                \
      for(i = 0;i < 16;i++)                                                                                                                          \
        interleaved_chunk_data[i][lane] = chunk_data[lane].i[i];                                                                                     \
      interleaved_hash[0][lane] = initial_H0;                                                                                                        \
      interleaved_hash[1][lane] = initial_H1;                                                                                                        \
      interleaved_hash[2][lane] = initial_H2;                                                                                                        \
      interleaved_hash[3][lane] = initial_H3;                                                                                                        \
      interleaved_hash[4][lane] = initial_H4;                                                                                                        \
    }                                                                                                                                                \
    /* do SHA_UPDATE on all lanes in one go */                                                                                                       \
    sha1_update_ ## EXTENSION((DATA_TYPE *)&interleaved_chunk_data[0],(DATA_TYPE *)&interleaved_hash[0]);                                            \
    /* test */                                                                                                                                       \
    for(lane = 0;lane < N_LANES;lane++)                                                                                                              \
      for(i = 0;i < 5;i++)                                                                                                                           \
        if(interleaved_hash[i][lane] != hash[lane].i[i])                                                                                             \
        {                                                                                                                                            \
          fprintf(stderr,"test_sha1_update_" # EXTENSION ": sha1_update_" # EXTENSION "() failure for n=%d (bad/good):\n",n);                        \
          for(i = 0;i < 5;i++)                                                                                                                       \
            for(lane = 0;lane < N_LANES;lane++)                                                                                                      \
              fprintf(stderr,"%s%08X/%08X%s",(lane == 0) ? "  " : " ",interleaved_hash[i][lane] ,hash[lane].i[i],(lane == N_LANES - 1) ? "\n" : ""); \
          exit(1);                                                                                                                                   \
        }                                                                                                                                            \
  }                                                                                                                                                  \
  /* measure */                                                                                                                                      \
  time_measurement();                                                                                                                                \
  sum = 0u;                                                                                                                                          \
  for(n = 0;n < n_measurements;n++)                                                                                                                  \
  {                                                                                                                                                  \
    interleaved_chunk_data[0][0]++;                                                                                                                  \
    sha1_update_ ## EXTENSION((DATA_TYPE *)&interleaved_chunk_data[0],(DATA_TYPE *)&interleaved_hash[0]);                                            \
    sum += interleaved_hash[4][0];                                                                                                                   \
  }                                                                                                                                                  \
  time_measurement();                                                                                                                                \
  if(sum == 0u)                                                                                                                                      \
    fprintf(stderr,"test_sha1_update_" # EXTENSION ": what a coincidence, sum=0\n");                                                                 \
  chunks_per_second = (double)n_measurements * (double)N_LANES / cpu_time_delta();                                                                   \
  /* report */                                                                                                                                       \
  printf("sha1_update_" # EXTENSION "() passed (%d test%s, %.0f chunks per second)\n",n_tests,(n_tests == 1) ? "" : "s",chunks_per_second);          \
}


//
// other tests
//

#if defined(__AVX__)
DO_TEST(avx,16,4,v4si)
#endif

#if defined(__AVX2__)
DO_TEST(avx2,32,8,v8si)
#endif

#if defined(__AVX512F__)
DO_TEST(avx512f,64,16,v16si)
#endif

#if defined(__ARM_NEON)
DO_TEST(neon,16,4,uint32x4_t)
#endif


//
// main program
//

int main(void)
{
  const int n_tests = 1000;
  const int n_measurements = 10000000;

  test_sha1_update(n_tests,n_measurements);
#if defined(__AVX__)
  test_sha1_update_avx(n_tests,n_measurements);
#endif
#if defined(__AVX2__)
  test_sha1_update_avx2(n_tests,n_measurements);
#endif
#if defined(__AVX512F__)
  test_sha1_update_avx512f(n_tests,n_measurements);
#endif
#if defined(__ARM_NEON)
  test_sha1_update_neon(n_tests,n_measurements);
#endif
  return 0;
}
