//
// test_engines.c --- critérios de aceitação do Bloco B (António): todos os motores contra o search_scalar
//
//   make test_engines        (sem CUDA: AVX, AVX2, AVX-512 se o CPU tiver, OpenMP)
//   make test_engines_cuda   (com CUDA; precisa de nvcc e de uma placa NVIDIA)
//   correr a partir do Projeto1:  ./bin/test_engines   ou   ./bin/test_engines_cuda
//
// Critérios do plano do grupo:
//  1. SIMD e OpenMP: h4_mask = 0xFF, max_found = 8192, 2^20 candidatos -> o CONJUNTO de contadores vencedores
//     é igual ao do search_scalar, em todos os tipos, com n_blocks = 1 e = 2 (os PPM têm 2 blocos);
//  2. CUDA: o mesmo com h4_mask = 0xFFF e 2^24 candidatos;
//  3. CUDA com h4_mask = 0x1 (cerca de metade dos candidatos são soluções): o total contado continua certo;
//  4. o valor devolvido (atualizações de bloco) = candidatos x n_blocks.
// As soluções do OpenMP e da CUDA podem vir por outra ordem (critical / atomicAdd), por isso compara-se o conjunto.
//

#include <stdio.h>
#include "aad_engines.h"
#include "aad_files.h"

#define MF 8192u

static u64_t ref_found[MF], got_found[MF];
static int falhas = 0;

static void ordenar(u64_t *v, u32_t n)   // insertion sort (poucas centenas/milhares de valores)
{
    for (u32_t a = 1; a < n; a++)
        for (u32_t b = a; b > 0 && v[b - 1] > v[b]; b--) {
            u64_t x = v[b]; v[b] = v[b - 1]; v[b - 1] = x;
        }
}

// compara o resultado de um motor com o do escalar (nº de blocos, total de soluções e o conjunto)
static void comparar(const char *nome, u64_t ref_upd, u32_t ref_n, u64_t upd, u32_t n, int so_total)
{
    int ok = (upd == ref_upd && n == ref_n);
    u32_t m = (n < MF) ? n : MF;

    if (ok && !so_total) {
        ordenar(got_found, m);
        for (u32_t j = 0; ok && j < m; j++)
            ok = (got_found[j] == ref_found[j]);
    }
    printf(" %s=%s", nome, ok ? "ok" : "FALHA");
    if (!ok)
        printf("(soluções %u/%u, blocos %lu/%lu)", n, ref_n, (unsigned long)upd, (unsigned long)ref_upd);
    falhas += !ok;
}

static void testar_ficheiro(const char *path)
{
    file_type_t ty;
    search_template_t t;
    u32_t ref_n, n;
    u64_t ref_upd, upd;
    const u64_t first = 1000003u;                 // ímpar: o count não é múltiplo do nº de lanes
    const u64_t count = (1u << 20) + 5u;

    if (file_type_from_path(path, &ty) != 0 || make_template(path, ty, &t) != 0) {
        printf("%s: não foi possível ler\n", path);
        falhas++;
        return;
    }

    // critério 1: SIMD e OpenMP
    t.h4_mask = 0xFFu;
    ref_upd = search_scalar(&t, first, count, ref_found, MF, &ref_n);
    printf("%-42s n_blocks=%u soluções=%u:", path, t.n_blocks, ref_n);
    if (__builtin_cpu_supports("avx"))     { upd = search_avx(&t, first, count, got_found, MF, &n);    comparar("avx", ref_upd, ref_n, upd, n, 0); }
    else printf(" avx=skip");
    if (__builtin_cpu_supports("avx2"))    { upd = search_avx2(&t, first, count, got_found, MF, &n);   comparar("avx2", ref_upd, ref_n, upd, n, 0); }
    else printf(" avx2=skip");
    if (__builtin_cpu_supports("avx512f")) { upd = search_avx512(&t, first, count, got_found, MF, &n); comparar("avx512", ref_upd, ref_n, upd, n, 0); }
    else printf(" avx512=skip");
    upd = search_omp(&t, first, count, got_found, MF, &n, 1);  comparar("omp-j1", ref_upd, ref_n, upd, n, 0);
    upd = search_omp(&t, first, count, got_found, MF, &n, 3);  comparar("omp-j3", ref_upd, ref_n, upd, n, 0);
    upd = search_omp(&t, first, count, got_found, MF, &n, 16); comparar("omp-j16", ref_upd, ref_n, upd, n, 0);

#if defined(AAD_CUDA)
    // critério 2: CUDA com 0xFFF e 2^24 candidatos
    t.h4_mask = 0xFFFu;
    ref_upd = search_scalar(&t, first, 1u << 24, ref_found, MF, &ref_n);
    upd = search_cuda(&t, first, 1u << 24, got_found, MF, &n);
    comparar("cuda-0xFFF", ref_upd, ref_n, upd, n, 0);

    // critério 3: CUDA com 0x1 (milhares de soluções por lançamento): o total tem de continuar certo
    t.h4_mask = 0x1u;
    ref_upd = search_scalar(&t, first, count, ref_found, MF, &ref_n);
    upd = search_cuda(&t, first, count, got_found, MF, &n);
    comparar("cuda-0x1", ref_upd, ref_n, upd, n, 1);
#endif
    printf("\n");
}

int main(void)
{
    static const char *const ficheiros[] = {
        "inputs/aad_a1.pdf", "inputs/aad_a1.png", "inputs/aad_a1.ppm", "inputs/aad_a1.html", "inputs/aad_a1.txt",
        "examples/deti_4_2_37_original.pdf", "examples/example_frame_001_original.png",
        "examples/example_frame_001.ppm",
    };

#if defined(AAD_CUDA)
    search_template_t t0;
    file_type_t ty0;
    file_type_from_path(ficheiros[0], &ty0);
    make_template(ficheiros[0], ty0, &t0);
    cuda_init(&t0);
#endif
    for (unsigned k = 0; k < sizeof(ficheiros) / sizeof(ficheiros[0]); k++)
        testar_ficheiro(ficheiros[k]);
#if defined(AAD_CUDA)
    cuda_done();
#endif
    printf("%s (%d falha%s)\n", falhas ? "HÁ FALHAS" : "todos os motores iguais ao escalar", falhas, falhas == 1 ? "" : "s");
    return falhas != 0;
}
