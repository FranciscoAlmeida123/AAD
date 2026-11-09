//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// test the SHA1 implementation (CUDA code)
//
// initialize_cuda: CUDA code running on a NVIDIA GeForce GTX 1660 Ti with Max-Q Design (device 0, CUDA 10.1.0)
//   sha1_cuda_kernel() passed for b=3D (8388608 tests,  2266211368 chunks per second)
//     host -> device --- 0.106117 seconds
//     kernel ----------- 0.003702 seconds
//     device -> host --- 0.025653 seconds
//   sha1_cuda_kernel() passed for b=1B (8388608 tests,  2317295028 chunks per second)
//     host -> device --- 0.106044 seconds
//     kernel ----------- 0.003620 seconds
//     device -> host --- 0.025580 seconds
//   sha1_cuda_kernel() passed for b=DF (8388608 tests,  2376712849 chunks per second)
//     host -> device --- 0.106065 seconds
//     kernel ----------- 0.003530 seconds
//     device -> host --- 0.025639 seconds
//   sha1_cuda_kernel() passed for b=BC (8388608 tests,  2385024451 chunks per second)
//     host -> device --- 0.106053 seconds
//     kernel ----------- 0.003517 seconds
//     device -> host --- 0.025637 seconds
//
// initialize_cuda: CUDA code running on a NVIDIA GeForce GTX 1660 Ti (device 0, CUDA 12.4.0)
//   sha1_cuda_kernel() passed for b=3D (8388608 tests,  2446392227 chunks per second)
//     host -> device --- 0.053785 seconds
//     kernel ----------- 0.003429 seconds
//     device -> host --- 0.012403 seconds
//   sha1_cuda_kernel() passed for b=1B (8388608 tests,  2456981196 chunks per second)
//     host -> device --- 0.053787 seconds
//     kernel ----------- 0.003414 seconds
//     device -> host --- 0.012401 seconds
//   sha1_cuda_kernel() passed for b=DF (8388608 tests,  2456534382 chunks per second)
//     host -> device --- 0.053784 seconds
//     kernel ----------- 0.003415 seconds
//     device -> host --- 0.012402 seconds
//   sha1_cuda_kernel() passed for b=BC (8388608 tests,  2456310677 chunks per second)
//     host -> device --- 0.053788 seconds
//     kernel ----------- 0.003415 seconds
//     device -> host --- 0.012401 seconds
//
// initialize_cuda: CUDA code running on a NVIDIA RTX A6000 (device 0, CUDA 12.8.0) --- 300W
//   sha1_cuda_kernel() passed for b=3D (8388608 tests,  6624168091 chunks per second)
//     host -> device --- 0.028228 seconds
//     kernel ----------- 0.001266 seconds
//     device -> host --- 0.006207 seconds
//   sha1_cuda_kernel() passed for b=1B (8388608 tests,  6661373741 chunks per second)
//     host -> device --- 0.028582 seconds
//     kernel ----------- 0.001259 seconds
//     device -> host --- 0.006204 seconds
//   sha1_cuda_kernel() passed for b=DF (8388608 tests,  6679705535 chunks per second)
//     host -> device --- 0.028540 seconds
//     kernel ----------- 0.001256 seconds
//     device -> host --- 0.006205 seconds
//   sha1_cuda_kernel() passed for b=BC (8388608 tests,  6686163641 chunks per second)
//     host -> device --- 0.028433 seconds
//     kernel ----------- 0.001255 seconds
//     device -> host --- 0.006204 seconds
//
// initialize_cuda: CUDA code running on a NVIDIA RTX PRO 5000 Blackwell (device 0, CUDA 12.8.0)
//   sha1_cuda_kernel() passed for b=3D (8388608 tests, 11721630066 chunks per second)
//     host -> device --- 0.026213 seconds
//     kernel ----------- 0.000716 seconds
//     device -> host --- 0.005872 seconds
//   sha1_cuda_kernel() passed for b=1B (8388608 tests, 11922596857 chunks per second)
//     host -> device --- 0.026454 seconds
//     kernel ----------- 0.000704 seconds
//     device -> host --- 0.005869 seconds
//   sha1_cuda_kernel() passed for b=DF (8388608 tests, 11946401742 chunks per second)
//     host -> device --- 0.026419 seconds
//     kernel ----------- 0.000702 seconds
//     device -> host --- 0.005869 seconds
//   sha1_cuda_kernel() passed for b=BC (8388608 tests, 11975804575 chunks per second)
//     host -> device --- 0.026385 seconds
//     kernel ----------- 0.000700 seconds
//     device -> host --- 0.005869 seconds
//
// initialize_cuda: CUDA code running on a NVIDIA RTX PRO 6000 Blackwell Max-Q Workstation Edition (device 0, CUDA 12.8.0) --- 300W
//   sha1_cuda_kernel() passed for b=3D (8388608 tests, 16036214596 chunks per second)
//     host -> device --- 0.012905 seconds
//     kernel ----------- 0.000523 seconds
//     device -> host --- 0.002898 seconds
//   sha1_cuda_kernel() passed for b=1B (8388608 tests, 16462615419 chunks per second)
//     host -> device --- 0.013470 seconds
//     kernel ----------- 0.000510 seconds
//     device -> host --- 0.002895 seconds
//   sha1_cuda_kernel() passed for b=DF (8388608 tests, 16475904656 chunks per second)
//     host -> device --- 0.012906 seconds
//     kernel ----------- 0.000509 seconds
//     device -> host --- 0.002895 seconds
//   sha1_cuda_kernel() passed for b=BC (8388608 tests, 16575656023 chunks per second)
//     host -> device --- 0.013133 seconds
//     kernel ----------- 0.000506 seconds
//     device -> host --- 0.002895 seconds
//
// note that transfer times are much slower than the actual computation time --- this has to be addressed in the A1 assignment
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aad_data_types.h"
#include "aad_utilities.h"
#include "aad_sha1_cpu.h"
#include "aad_cuda_utilities.h"

//
// test the CUDA implementation
//

static void test_sha1_cuda(u32_t n_tests,u32_t n_passes)
{
  u32_t pass,n,*interleaved32_chunk_data,*interleaved32_hash,chunk_data[16],hash[5],good_hash[5];
  double host_to_device_time,kernel_time,device_to_host_time,chunks_per_second;
  u08_t b,seen[256];
  cuda_data_t cd;

  if(n_tests <= 0 || n_tests > (1 << 24) || n_tests % RECOMENDED_CUDA_BLOCK_SIZE != 0 || n_passes < 1u || n_passes > 256u)
  {
    fprintf(stderr,"test_sha1_cuda(): bad number of tests or or passes\n");
    exit(1);
  }
  // initialize
  cd.device_number = 0; // first device
  cd.cubin_file_name = "sha1_cuda_kernel.cubin";
  cd.kernel_name = "sha1_update_cuda_kernel";
  cd.data_size[0] = (u32_t)n_tests * (u32_t)16 * (u32_t)sizeof(u32_t); // size of the data array
  cd.data_size[1] = (u32_t)n_tests * (u32_t) 5 * (u32_t)sizeof(u32_t); // size of the hash array
  fprintf(stderr,"test_sha1_cuda(): %.3f MiB bytes for the interleaved32_chunk_data[] array\n",(double)cd.data_size[0] / (double)(1 << 20));
  fprintf(stderr,"test_sha1_cuda(): %.3f MiB bytes for the interleaved32_hash[] array\n",(double)cd.data_size[1] / (double)(1 << 20));
  initialize_cuda(&cd);
  interleaved32_chunk_data = (u32_t *)cd.host_data[0];
  interleaved32_hash = (u32_t *)cd.host_data[1];
  for(n = 0;n < 256;n++)
    seen[n] = 0;
  for(pass = 0u;pass < n_passes;pass++)
  {
    do
      b = random_byte(); // random byte used to initialize all hash bytes
    while(seen[(size_t)b] != (u08_t)0);
    seen[(size_t)b] = (u08_t)1;
    // random interleaved32_chunk_data
    n = cd.data_size[0];
    while(n != 0u)
      ((u08_t *)interleaved32_chunk_data)[--n] = random_byte();
    // initial hash data (all bytes equal to b)
    n = cd.data_size[1];
    while(n != 0u)
    ((u08_t *)interleaved32_hash)[--n] = b;
    // run SHA1_UPDATE on the CUDA device
    time_measurement();
    host_to_device_copy(&cd,0); // idx=0 means that the interleaved32_chunk_data is copied to the CUDA device
    host_to_device_copy(&cd,1); // idx=1 means that the interleaved32_hash is copied to the CUDA device
    time_measurement();
    host_to_device_time = wall_time_delta();
    cd.grid_dim_x = (u32_t)n_tests / (u32_t)RECOMENDED_CUDA_BLOCK_SIZE;
    cd.block_dim_x = (u32_t)RECOMENDED_CUDA_BLOCK_SIZE;
    cd.n_kernel_arguments = 2;
    cd.arg[0] = &cd.device_data[0]; // interleaved32_data
    cd.arg[1] = &cd.device_data[1]; // interleaved32_hash
    time_measurement();
    launch_kernel(&cd);
    time_measurement();
    kernel_time = wall_time_delta();
    time_measurement();
    device_to_host_copy(&cd,1); // idx=1 means that the interleaved32_hash is copied to the host
    time_measurement();
    device_to_host_time = wall_time_delta();
    // test
    for(n = 0;n < n_tests;n++)
    {
      // deinterleave the data and the hash
      // on the CUDA side, the data for each warp is clustered together; what follows must match what is in the CUDA kernel
      // each warp has 32 threads
      int warp_number = n / 32;
      int lane = n % 32;
      for(int idx = 0;idx < 16;idx++)
        chunk_data[idx] = interleaved32_chunk_data[32 * 16 * warp_number + 32 * idx + lane];
      for(int idx = 0;idx <  5;idx++)
        hash[idx] = interleaved32_hash[32 *  5 * warp_number + 32 * idx + lane];
      // compute SHA1_UPDATE on the cpu
      for(int idx = 0;idx <  5;idx++)
        good_hash[idx] = 0x01010101u * (u32_t)b;
      sha1_update(&chunk_data[0],&good_hash[0]);
      // compare them
      for(int idx = 0;idx < 5;idx++)
        if(hash[idx] != good_hash[idx])
        {
          fprintf(stderr,"test_sha1_cuda() failed for n=%d with b=%02X\n",n,(u32_t)b);
          for(idx = 0;idx < 16;idx++)
            fprintf(stderr,"%2d 0x%08X\n",idx,chunk_data[idx]);
          fprintf(stderr,"---\n");
          for(idx = 0;idx < 5;idx++)
            fprintf(stderr,"%2d 0x%08X 0x%08X\n",idx,good_hash[idx],hash[idx]);
          exit(1);
        }
    }
    chunks_per_second = (double)n_tests / kernel_time;
    printf("sha1_cuda_kernel() passed for b=%02X (%d test%s, %.0f chunks per second)\n",(u32_t)b,n_tests,(n_tests == 1) ? "" : "s",chunks_per_second);
    printf("  host -> device --- %.6f seconds\n",host_to_device_time);
    printf("  kernel ----------- %.6f seconds\n",kernel_time);
    printf("  device -> host --- %.6f seconds\n",device_to_host_time);
  }
  // cleanup
  terminate_cuda(&cd);
}


//
// main program
//

int main(void)
{
  const u32_t n_tests = 128u * 65536u;
  const u32_t n_passes = 4u;

  test_sha1_cuda(n_tests,n_passes);
  return 0;
}
