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
	rm -rf bin


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


#
# ======== our project (src/, tests/) --- the rules above are the teacher's ========
#
#   make            all engines (stubs where not yet written)
#   make test       builds and runs the tests
#   make portable   scalar engine only (Raspberry Pi, phones): bin/aad_modify_portable
#   make clean      (the teacher's clean, which also removes bin/)
#

.DEFAULT_GOAL := all

CC      = gcc
CFLAGS  = -std=c11 -O3 -Wall -Wextra -I . -I src
LDLIBS  =
BIN     = bin

COMMON  = src/aad_files.c src/aad_scalar.c
HEADERS = $(wildcard src/*.h) aad_sha1.h aad_sha1_cpu.h aad_data_types.h aad_png_crc.h

# each SIMD engine is compiled with ITS flags only (no -march=native), chosen at run time
ENGINE_OBJS = $(BIN)/aad_avx.o $(BIN)/aad_avx2.o $(BIN)/aad_avx512.o $(BIN)/aad_omp.o $(BIN)/aad_cuda_host.o

.PHONY: all test portable clean

all: $(BIN)/aad_modify $(BIN)/test_bloco_a

$(BIN):
	mkdir -p $(BIN)

$(BIN)/aad_avx.o:       src/aad_avx.c $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) -mavx -c $< -o $@
$(BIN)/aad_avx2.o:      src/aad_avx2.c $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) -mavx2 -c $< -o $@
$(BIN)/aad_avx512.o:    src/aad_avx512.c $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) -mavx512f -c $< -o $@
$(BIN)/aad_omp.o:       src/aad_omp.c $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) -mavx2 -fopenmp -c $< -o $@
$(BIN)/aad_cuda_host.o: src/aad_cuda_host.c $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/aad_modify: src/aad_modify.c $(COMMON) $(ENGINE_OBJS) $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) src/aad_modify.c $(COMMON) $(ENGINE_OBJS) -o $@ -fopenmp $(LDLIBS)

$(BIN)/test_bloco_a: tests/test_bloco_a.c $(COMMON) $(HEADERS) | $(BIN)
	$(CC) $(CFLAGS) tests/test_bloco_a.c $(COMMON) -o $@

test: $(BIN)/test_bloco_a $(BIN)/aad_modify
	./$(BIN)/test_bloco_a
	bash tests/test_cli.sh

portable: | $(BIN)
	$(CC) $(CFLAGS) -DAAD_PORTABLE src/aad_modify.c $(COMMON) -o $(BIN)/aad_modify_portable
