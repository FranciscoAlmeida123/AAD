#
# Tomás Oliveira e Silva,  September 2026
#
# Arquiteturas de Alto Desempenho 2026/2027
#
# makefile for the first practical assignment A1 (2026/2027)
#
# makefile automatic variables:
#   $@ is the name of the target
#   $< is the name of the first prerequisite
#   $^ is the list of names of all prerequisites (without duplicates)
#

#
# CUDA installation directory --- /usr/local/cuda or $(CUDA_HOME)
#

CUDA_DIR = /usr/local/cuda


#
# OpenCL installation directory (for a NVidia graphics card, sama as CUDA)
#

OPENCL_DIR = $(CUDA_DIR)


#
# CUDA device architecture
#
#   GeForce GTX 1660 Ti --------------------------------------- sm_75
#   RTX A2000 Ada --------------------------------------------- sm_86
#   RTX A6000 Ada --------------------------------------------- sm_86
#   RTX 4070 -------------------------------------------------- sm_89
#   NVIDIA RTX PRO 6000 Blackwell Max-Q Workstation Edition --- sm_120
#

CUDA_ARCH = sm_75


#
# clean up
#

clean:
	rm -f sha1_cpu_tests
	rm -f sha1_cuda_test sha1_cuda_kernel.cubin
	rm -f parse_png
	rm -f a.out


#
# test the SHA1_UPDATE macro
#

sha1_cpu_tests:	aad_sha1_cpu_tests.c aad_sha1.h aad_data_types.h aad_utilities.h makefile
	cc -march=native -Wall -Wextra -Wshadow -Werror -O3 $< -o $@

sha1_cuda_test:	aad_sha1_cuda_test.c sha1_cuda_kernel.cubin aad_sha1.h aad_data_types.h aad_utilities.h aad_cuda_utilities.h makefile
	cc -march=native -Wall -Wextra -Wshadow -Werror -O3 $< -o $@ -lcuda


#
# compile the CUDA kernels
#

sha1_cuda_kernel.cubin:	aad_sha1_cuda_kernel.cu aad_sha1.h makefile
	nvcc -arch=$(CUDA_ARCH) --compiler-options -O2,-Wall,-Wextra -I$(CUDA_DIR)/include --cubin $< -o $@


#
# show the structure of PNG files
#
parse_png:	parse_png.c aad_png_crc.h
	cc -march=native -Wall -Wextra -Wshadow -Werror -O3 $< -o $@
