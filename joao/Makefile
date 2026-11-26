# AAD A1 --- our Makefile (the teacher files stay untouched in ../)
#
#   make            all engines (stubs where not yet written)
#   make test       builds and runs the tests
#   make portable   scalar engine only (Raspberry Pi, phones): bin/aad_modify_portable
#   make clean

CC      = gcc
CFLAGS  = -std=c11 -O3 -Wall -Wextra -I .. -I src
LDLIBS  =
BIN     = bin

COMMON  = src/aad_files.c src/aad_scalar.c
HEADERS = $(wildcard src/*.h) ../aad_sha1.h ../aad_sha1_cpu.h ../aad_data_types.h ../aad_png_crc.h

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

clean:
	rm -rf $(BIN) *.o *.cubin *.ptx
