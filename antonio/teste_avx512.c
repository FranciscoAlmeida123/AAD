#include <stdio.h>
#include "aad_data_types.h"   // tipos v16si, u32_t
#include "aad_sha1.h"         // H0..H4 e a macro
#include "aad_sha1_cpu.h"     // sha1_update e sha1_update_avx
# define MASK 0x000FFFFFu
#include <time.h>

// Mudar quando tiver counter e limpar o codigo depois
static void meter_contador(u32_t data[16][16], int lane, u32_t contador)
{
    data[0][lane] = contador;    
}                                


// Guião P1 (usa clock que não volta atrás)
static double agora(void)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec + 1.0e-9 * (double)t.tv_nsec;   // segundos, com casas decimais
}

int main(void){

    // Parte 1: calcular midstate

    u32_t bloco1[16];  // 16 palavras = 64 bytes
    u32_t mid[5];      // midstate: H0..H4 de 32 bits
    int i;

    // encher o bloco1 com valores inventados
    for(i=0; i < 16;i++)
        bloco1[i] = i* 1000;

    // estado inicial do SHA1
    mid[0] = initial_H0;
    mid[1] = initial_H1;
    mid[2] = initial_H2;
    mid[3] = initial_H3;
    mid[4] = initial_H4;

    // calcular o midstate uma única vez
    sha1_update(bloco1,mid);

    // mostrar o midstate
    for(i=0; i<5; i++)
        printf("mid[%d] = %08X\n",i,mid[i]);

    // Parte 2 : AVX512 , 16 lanes


    u32_t bloco2[16];
    // AVX512 = 512 bits = 64 bytes
    u32_t data[16][16] __attribute__((aligned(64))); // 16 plavras x 16 lanes
    u32_t hash[5][16] __attribute__((aligned(64))); // 5 midstate x 16 lanes
    int lane, j;
    u32_t counter = 0;

    int found = 0;          // soluções encontradas
    u32_t blk[16], h[5];    //

    for (i = 0; i <16; i++)
        bloco2[i] =  0xAAD00000u + i; 


    // entrelaçar a palavra i do candidato, palavra a palavra 
        for (i = 0; i<16; i++)
            for(lane = 0; lane < 16; lane++)
                data[i][lane] = bloco2[i];

    // Repetir de 16 em  16 lanes
    for(counter = 0; found < 5; counter +=16 )
    {

        // lane k testa o contador counter + k 
        for(lane = 0; lane <16; lane++)
            data[0][lane] = counter + lane;
        

        // midstate para as 16 lanes 
        for(j = 0 ; j < 5; j++)
            for(lane = 0; lane < 16; lane++)
                hash[j][lane] = mid[j];

        sha1_update_avx512f((v16si*)data,(v16si*)hash);

        // escalar serve apenas para conferir
        for (lane= 0; lane<16; lane++)
            if((hash[4][lane] & MASK) == 0) // condiçao se forem 0 todos 32 bits
            {
                       
                for (i = 0; i<16; i++)
                    blk[i]= bloco2[i];
                
                blk[0] = counter + lane ;

                
                for(j = 0 ; j < 5; j++)
                     h[j] = mid[j];

                sha1_update(blk, h);

                printf("contador %u: AVX2 %08x | escalar %08x  %s\n",
                       counter + lane, hash[4][lane], h[4],
                       (h[4] == hash[4][lane]) ? "OK" : "ERRO");
                found++;
            }

    }

    //  Parte 3: medir 
    #define SEGUNDOS 10.0
    double t0;
    unsigned long long tent_avx = 0, tent_esc = 0;
    u32_t lixo = 0;                 

    //  AVX512
    t0 = agora();
    for(counter = 0; ; counter += 16)
    {
        for(lane = 0; lane < 16; lane++)
            data[0][lane] = counter + lane;
        for(j = 0; j < 5; j++)
            for(lane = 0; lane < 16; lane++)
                hash[j][lane] = mid[j];
        sha1_update_avx512f((v16si *)data, (v16si *)hash);
        lixo ^= hash[4][0];
        tent_avx += 16;                                   
        if((counter & 0xFFFFF) == 0 && agora() - t0 >= SEGUNDOS)
            break;
    }

    // escalar
    for(i = 0; i < 16; i++)
        blk[i] = bloco2[i];
    t0 = agora();
    for(counter = 0; ; counter += 1)                     
    {
        blk[0] = counter;
        for(j = 0; j < 5; j++)
            h[j] = mid[j];
        sha1_update(blk, h);
        lixo ^= h[4];
        tent_esc += 1;
        if((counter & 0xFFFFF) == 0 && agora() - t0 >= SEGUNDOS)
            break;
    }

    printf("AVX512:     %.0f tentativas/s\n", tent_avx / SEGUNDOS);
    printf("escalar: %.0f tentativas/s\n", tent_esc / SEGUNDOS);
    printf("speedup: %.2fx\n", (double)tent_avx / (double)tent_esc);
    printf("(lixo = %08x)\n", lixo);

    return 0;


}
