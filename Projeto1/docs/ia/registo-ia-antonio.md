# Registo de utilização de IA — António (Bloco B: SIMD, OpenMP, CUDA)

Ferramenta: Claude (Anthropic), usada no Claude Code (desktop).

---

## estudo e testes dos motores SIMD e CUDA

**Problema:** Precisava de perceber o SHA1, o midstate, o SIMD/AVX, o OpenMP e a CUDA, e de escrever os primeiros testes.

**O que a IA fez:** explicou a matéria a partir dos guiões (P1, P3, P6) e do enunciado. Apoio guiado e verificação passo a passo do meu código de AVX, um bocado de cada vez. Reviu os meus erros, mas não reescreveu o código.

**O que escrevi:** `antonio/teste_avx.c`, `teste_avx2.c`, `teste_avx512.c` (midstate, entrelaçamento, 4/8/16 lanes, verificação com o escalar, medição) e `antonio/teste_cuda_kernel.cu` (kernel com `storage_area` e `atomicAdd`).

**Erros meus que a IA apontou:**
- `hash[5][lane]` fora do array (deu zeros falsos);
- chamar `sha1_update_avx` no ficheiro AVX2;
- compilar com `-mavx` em vez de `-mavx2`;
- `counter = 1 + n` em vez de `base + n` no kernel.

**Como verifiquei:** cada solução AVX/AVX2/AVX-512 foi conferida com o `sha1_update` escalar e deu OK. Os mesmos 5 contadores apareceram nas 3 versões. Testei o AVX-512 no Google Colab, porque o meu CPU não tem AVX-512.

**O que aprendi:** midstate, entrelaçamento, alinhamento, passagem de arrays por ponteiro, porque o speedup não chega ao nº de lanes (falta de instrução de rotação no AVX/AVX2) e o `atomicAdd` na CUDA.

---

## integração dos motores AVX/AVX2/AVX-512 com o código do João

**Problema:** passar os meus testes para os ficheiros do projeto (`src/aad_avx.c`, `aad_avx2.c`, `aad_avx512.c`), com o template do João (`build_candidate`, `t->midstate`, `t->h4_mask`, `n_blocks` 1 ou 2).

**O que ficou em `src/aad_avx.c`:** o meu `teste_avx.c` com as mudanças mínimas para funcionar no projeto:
- a função passou a chamar-se `search_avx(...)`, com os parâmetros combinados no grupo;
- a minha função `meter_contador` passou a meter na lane o candidato verdadeiro (`build_candidate` do João), em vez de um número na palavra 0;
- `mid` → `t->midstate`, `MASK` → `t->h4_mask`, `printf` → `found[]`;
- suporte a 2 blocos (PPM).

Defini estes passos com a IA, e a IA aplicou-os ao meu ficheiro a meu pedido.

**O que a IA propôs e eu não usei:** uma versão em que só se reescrevem as palavras onde caem os 11 caracteres do contador, em vez de copiar o bloco inteiro em cada lane. Preferi a minha versão porque a sei explicar, e na medição a diferença de velocidade foi pequena.

**Como verifiquei:**
- comparação com o `search_scalar` em 8 ficheiros (PDF, PNG, PPM, HTML, TXT e os exemplos do docente, incluindo PPM com 2 blocos), com `h4_mask = 0xFF` e ~1 milhão de candidatos: mesmos vencedores, pela mesma ordem;
- `make test` (657 verificações, 0 falhas);
- `objdump` ao `aad_avx.o` (não tem instruções AVX2).

**AVX2 e AVX-512:** `src/aad_avx2.c` e `src/aad_avx512.c` passaram para o mesmo estilo, a meu pedido. São o `aad_avx.c` com 8/16 lanes, `aligned(32)`/`aligned(64)`, `v8si`/`v16si` e `sha1_update_avx2`/`sha1_update_avx512f`, como nos meus testes `teste_avx2.c` e `teste_avx512.c`.

**Medição (TXT, 10 s, 1 núcleo, Ryzen 7 5700U):** escalar 12,4 M/s, AVX 22,5 M/s (1,8×), AVX2 25,6 M/s (2,1×). Com a versão otimizada que não usei, o AVX2 dava 31,5 M/s (2,5×). Construir o candidato inteiro em cada lane pesa mais com 8 lanes.

---

## organização do repositório

**O que a IA fez a meu pedido:**
- moveu `joao/` e `antonio/` para a raiz do repositório;
- juntou no `Projeto1/` o projeto do João com os meus motores;
- acrescentou as regras de compilação do grupo no fim do `makefile` do docente.

**Como verifiquei:** `make` sem avisos e `make test` sem falhas.

---

## erro da IA que encontrei: o `#pragma` não era preciso

**Problema:** ao limpar o meu `src/aad_avx.c`, tirei 1 linha que achei que não usava: `#pragma GCC diagnostic ignored "-Wunused-function"`.

**O que a IA tinha dito:** que o `#pragma` era necessário para evitar avisos "defined but not used". A IA tinha me dito para o por, sem testar.

**O que eu achei:** que eu nunca tinha visto isto e que pragma` não era importante. Discuti isto com a IA.

**Como verifiquei:**compilei o `aad_avx.c` de 2 formas, com as opções do `make` (`-Wall -Wextra`): sem `#pragma` e com `#pragma`. As duas compilam sem nenhum aviso.

**Estava certo?** Eu estava certo: o `#pragma` não é preciso. 

**O que mudei:** O `#pragma` pode sair também do `aad_avx2.c` e do `aad_avx512.c`.

**O que aprendi:** não aceitar uma sugestão da IA sem a testar. Um include pode parecer "não usado" e ser preciso por causa de uma função que vem de dentro dele.

---

## 2 OpenMP e CUDA

**Problema:** fazer o `search_omp` e o motor CUDA da forma mais simples, como nos guiões do professor.

**O que pedi à IA:** Apoio guiado e verificação passo a passo do meu código de AVX, um bocado de cada vez. Reviu os meus erros, mas não reescreveu o código. O código de `src/aad_omp.c`, `src/aad_search_kernel.cu` e `src/aad_cuda_host.c`, e a regra `make cuda` no makefile, foram escritos por mim com ajuda da IA.

**Como verifiquei (OpenMP):**
- com 1, 3 e 16 threads, o conjunto de vencedores é igual ao do `search_scalar` nos 8 ficheiros;
- encontrou um PDF real com SHA1 a acabar em `00000000` em 18 s;
- velocidade com 16 threads: 169 M/s (cerca de 14× o escalar).

**Ainda por verificar (CUDA):** o meu PC não tem NVIDIA. Tenho de testar no Google Colab (T4) com `make cuda`.

---

## teste no Google Colab (Tesla T4, CUDA 13.0, CPU Xeon com 2 vCPUs e AVX-512)

**Como foi feito:**  Escolhi a T4, corri um script que a I deu e que compila e testa tudo, e deu me os resultados.

**Problema encontrado:** o kernel compilou à primeira, mas o host CUDA não. No CUDA 13 o `cuCtxCreate` tem mais um argumento, e o `aad_cuda_utilities.h` do professor foi escrito para versões anteriores.

**O que a IA propôs:** não alterar o ficheiro do professor. No `src/aad_cuda_host.c`, antes de o incluir, fica `#define cuCtxCreate(ctx, flags, dev) cuCtxCreate_v4(ctx, NULL, flags, dev)`, só quando `CUDA_VERSION >= 13000` (NULL = parâmetros por omissão). Com isto, compilou.

**Resultado do teste de aceitação:** em todos os 8 ficheiros (incluindo os PPM com 2 blocos), com `h4_mask = 0xFF` e ~1 milhão de candidatos, o conjunto de vencedores é igual ao do `search_scalar` para todos os motores: AVX, AVX2, **AVX-512**, OpenMP (1, 3 e 16 threads) e **CUDA**.

**Medições no Colab (ficheiro TXT, 10 s cada):**

| motor | candidatos/s | vs escalar |
|---|---|---|
| escalar | 6,0 M | 1× |
| AVX | 12,6 M | 2,1× |
| AVX2 | 16,4 M | 2,7× |
| AVX-512 | 19,8 M | 3,3× |
| OpenMP (2 threads) | 25,7 M | 4,3× |
| **CUDA (T4)** | **1452 M** | **~240×** |

Com a CUDA, em 10 s apareceram 4 soluções com 32 bits a zero. Em média é uma solução a cada ~3 s, contra ~12 min com o escalar no Colab.

**2.º teste (nova sessão do Colab, com a correção já no projeto):**
- `make`, `make test` e `make cuda` passam sem avisos nem erros;
- teste de aceitação: todos os motores (incluindo CUDA e OpenMP) iguais ao escalar nos 8 ficheiros;
- OpenMP (2 CPUs no Colab): 25,9 M/s, contra 6,1 M/s do escalar (4,3×). Cada thread usa AVX-512, porque o CPU do Colab o tem;
- CUDA: 1439 M/s (~240×);
- **ficheiros reais modificados com CUDA**, todos com SHA1 a acabar em `00000000` (confirmado com `sha1sum`):

| ficheiro | contador | tempo |
|---|---|---|
| PDF | 3136591490 | 2,24 s |
| PNG | 1381880085 | 1,08 s |
| PPM (2 blocos) | 499390416 | 1,13 s |
| HTML | 210421544 | 0,15 s |
| TXT | 375741694 | 0,26 s |

O PDF e o TXT deram o mesmo contador (e o mesmo SHA1) que o OpenMP no meu PC. Isto confirma que a CUDA e o CPU testam os candidatos da mesma forma.

**O que aprendi:**
- o código do professor pode deixar de compilar numa versão nova da CUDA; resolve-se do nosso lado, sem alterar o ficheiro dele;
- a GPU é muito mais rápida porque cada thread testa um candidato e só as soluções voltam ao CPU (`storage_area`).

---

##  testes de aceitação do plano do grupo (Bloco B)

**Problema:** o plano do grupo tinha critérios de aceitação que ainda não estavam todos testados nem dentro do repositório: CUDA com `0xFFF` e 2^24 candidatos, CUDA com `0x1`, e o teste CUDA do professor (tarefa 0).

**O que a IA fez a meu pedido:**
- `tests/test_engines.c`: compara todos os motores com o `search_scalar`;
- `tests/cuda13_compat.h`: deixa compilar o `aad_sha1_cuda_test.c` do professor no CUDA 13 sem alterar o ficheiro dele (com `-include`);
- regras `test_engines`, `test_engines_cuda` e `sha1_cuda_test_13` no fim do `makefile`.

**Resultado no Google Colab (Tesla T4, CUDA 13.0):**
- **teste CUDA do professor:** `sha1_cuda_kernel() passed` 4 vezes, ~2,1–2,4 mil milhões de blocos/s. As transferências demoram muito mais do que o kernel: host→device 0,057 s e device→host 0,013 s, contra 0,004 s de kernel. É por isso que a secção 7 do enunciado manda transferir o mínimo, e o nosso kernel só devolve a `storage_area`;
- **teste de aceitação** (`make test_engines_cuda`): em todos os 8 ficheiros (incluindo os PPM com 2 blocos), AVX, AVX2, AVX-512, OpenMP (1, 3 e 16 threads), **CUDA com `0xFFF` e 2^24 candidatos** e **CUDA com `0x1`** (o total de soluções continua certo) dão o mesmo que o escalar: `todos os motores iguais ao escalar (0 falhas)`.

**No meu PC:** `make test_engines` também passa (sem AVX-512, porque o meu CPU não o tem).
