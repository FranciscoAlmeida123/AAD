
#include "aad_data_types.h"   // tipos v8si, u32_t
#include "aad_sha1.h"         // H0..H4 e a macro
#include "aad_sha1_cpu.h"     // sha1_update e sha1_update_avx2
#include "aad_engines.h"      // o nome da tua função e o search_template_t
#include "aad_mutate.h"       // build_candidate (o candidato verdadeiro)


// mete na lane o candidato verdadeiro (o fim do ficheiro com este contador lá escrito)
static void meter_contador(u32_t data[32][8], int lane, const search_template_t *t, u64_t contador)
{
    u32_t cand[32];
    int i;
    build_candidate(t, contador, cand);
    for (i = 0; i < 16 * (int)t->n_blocks; i++)
        data[i][lane] = cand[i];              // igual ao meu entrelaçamento
}


u64_t search_avx2(const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t max_found, u32_t *n_found)
{
    // AVX2 , 8 lanes

    // AVX2 = 256 bits = 32 bytes
    u32_t data[32][8] __attribute__((aligned(32))); // 32 palavras (1 ou 2 blocos) x 8 lanes
    u32_t hash[5][8] __attribute__((aligned(32)));  // 5 midstate x 8 lanes
    int lane, j;
    u64_t counter;

    *n_found = 0;           // soluções encontradas

    // Repetir de 8 em 8 lanes
    for (counter = 0; counter < count; counter += 8)
    {
        // lane k testa o contador first + counter + k
        for (lane = 0; lane < 8; lane++)
            meter_contador(data, lane, t, first + counter + lane);

        // midstate para as 8 lanes
        for (j = 0; j < 5; j++)
            for (lane = 0; lane < 8; lane++)
                hash[j][lane] = t->midstate[j];          // era mid[j]

        sha1_update_avx2((v8si*)data, (v8si*)hash);
        if (t->n_blocks == 2)                             // 2.º bloco continua do hash do 1.º
            sha1_update_avx2((v8si*)data[16], (v8si*)hash);

        for (lane = 0; lane < 8; lane++)
            if (counter + lane < count && (hash[4][lane] & t->h4_mask) == 0)   // era MASK
            {
                if (*n_found < max_found)
                    found[*n_found] = first + counter + lane;                  // era o printf
                (*n_found)++;
            }
    }
    return count * t->n_blocks;
}
