// aad_omp.c --- motor OpenMP: várias threads, cada uma corre o meu motor AVX2. Autor: António


#include <omp.h>
#include "aad_engines.h"      // search_avx2 e o search_template_t

#define MAX_MEUS 8192         // máximo de soluções que cada thread guarda

u64_t search_omp(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found,
                 int n_threads)
{
    u64_t blocos = 0;         // partilhada: cada thread soma na sua cópia e o reduction junta no fim
    u32_t guardadas = 0;      // partilhada: só se muda dentro do critical

    *n_found = 0;

    // "# pragma omp parallel num_threads(8)" -> todas as threads correm este bloco
#   pragma omp parallel num_threads(n_threads) reduction(+:blocos)
    {
        // variáveis declaradas aqui dentro são privadas (uma cópia por thread)
        int id = omp_get_thread_num();               // número desta thread: 0, 1, 2, ...
        int nt = omp_get_num_threads();              // quantas threads há
        // o pedaço desta thread: os pedaços não se sobrepõem e juntos dão [first, first + count)
        u64_t inicio = first + count * (u64_t)id / (u64_t)nt;
        u64_t fim    = first + count * (u64_t)(id + 1) / (u64_t)nt;
        u64_t meus[MAX_MEUS];                        // soluções desta thread
        u32_t n_meus, k;
        
        // usar o melhor SIMD: se o CPU tiver AVX-512 usa esse, senão AVX2
        
        if (__builtin_cpu_supports("avx512f"))

            blocos += search_avx512(t, inicio, fim - inicio, meus, MAX_MEUS, &n_meus);
        else

            blocos += search_avx2(t, inicio, fim - inicio, meus, MAX_MEUS, &n_meus);

        // P3: critical -> só uma thread de cada vez junta as suas soluções às partilhadas
#       pragma omp critical
        {
            for (k = 0; k < n_meus && k < MAX_MEUS; k++)
                if (guardadas < max_found)
                    found[guardadas++] = meus[k];
            *n_found += n_meus;
        }
    }   // fim do bloco = barreira: todas as threads acabaram

    return blocos;
}
