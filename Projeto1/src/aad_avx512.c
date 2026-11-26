//
// aad_avx512.c --- motor AVX-512: 16 lanes, 1 núcleo. Autor: António
//
// É o meu teste antonio/teste_avx512.c com o template do João. Igual ao aad_avx.c com:
//   4 -> 16 lanes, aligned(16) -> aligned(64), v4si -> v16si, sha1_update_avx -> sha1_update_avx512f.
// Compila só com -mavx512f (ver o makefile). O meu PC não tem AVX-512: testa-se no Colab ou no laboratório.
//

#pragma GCC diagnostic ignored "-Wunused-function"   // aad_sha1_cpu.h tem funções que aqui não uso

#include <string.h>
#include "aad_engines.h"
#include "aad_mutate.h"
#include "aad_sha1_cpu.h"     // sha1_update_avx512f

u64_t search_avx512(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
    u32_t bloco[32];                                   // o(s) bloco(s) do fim do ficheiro: 1 ou 2 x 16 palavras
    u08_t bytes[128];                                  // os mesmos blocos em bytes (onde se escreve o contador)
    // AVX-512 = 512 bits = 64 bytes
    u32_t data[32][16] __attribute__((aligned(64)));  // 32 palavras x 16 lanes
    u32_t hash[5][16]  __attribute__((aligned(64)));  // H0..H4 x 16 lanes
    u32_t n_palavras = 16 * t->n_blocks;
    // palavras que mudam de candidato para candidato (as outras são iguais em todos)
    u32_t fim = (t->type == FT_PNG) ? t->crc_off + 4 : t->var_off + t->var_len;
    u32_t p0 = t->var_off / 4, p1 = (fim - 1) / 4;
    u32_t i, j, lane, b;
    u64_t counter;

    // entrelaçar a palavra i do candidato em todas as lanes (uma vez, antes do ciclo)
    build_candidate(t, first, bloco);
    for (i = 0; i < n_palavras; i++)
        for (lane = 0; lane < 16; lane++)
            data[i][lane] = bloco[i];
    memcpy(bytes, t->tail, sizeof(bytes));

    *n_found = 0;
    // repetir de 16 em 16 (16 lanes)
    for (counter = 0; counter < count; counter += 16)
    {
        // lane k testa o contador first + counter + k (só as palavras p0..p1 mudam)
        for (lane = 0; lane < 16; lane++)
        {
            aad_apply_counter(t, first + counter + lane, bytes);
            for (i = p0; i <= p1; i++)   // 4 bytes -> 1 palavra big-endian (a ordem que a macro quer)
                data[i][lane] = (u32_t)bytes[4 * i] << 24 | (u32_t)bytes[4 * i + 1] << 16 |
                                (u32_t)bytes[4 * i + 2] << 8 | (u32_t)bytes[4 * i + 3];
        }

        // midstate para as 16 lanes (a função escreve por cima do hash, por isso repõe-se sempre)
        for (j = 0; j < 5; j++)
            for (lane = 0; lane < 16; lane++)
                hash[j][lane] = t->midstate[j];

        // 1 ou 2 blocos: o 2.º continua do hash do 1.º
        for (b = 0; b < t->n_blocks; b++)
            sha1_update_avx512f((v16si *)data[16 * b], (v16si *)hash);

        // sucesso: os bits da máscara do H4 estão a zero (a última volta pode ter lanes a mais)
        for (lane = 0; lane < 16; lane++)
            if (counter + lane < count && (hash[4][lane] & t->h4_mask) == 0)
            {
                if (*n_found < max_found)
                    found[*n_found] = first + counter + lane;
                (*n_found)++;
            }
    }
    return count * t->n_blocks;   // nº de blocos calculados (igual ao escalar)
}
