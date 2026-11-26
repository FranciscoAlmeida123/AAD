# Registo de utilização de IA — António (Bloco B: SIMD, OpenMP, CUDA)

Ferramenta: Claude (Anthropic), usada no Claude Code (desktop).

---

## 22/11/2026 — estudo e testes dos motores SIMD e CUDA

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

## 26/11/2026 — integração dos motores AVX/AVX2/AVX-512 com o código do João

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

## 26/11/2026 — organização do repositório

**O que a IA fez a meu pedido:**
- moveu `joao/` e `antonio/` para a raiz do repositório;
- juntou no `Projeto1/` o projeto do João com os meus motores;
- acrescentou as regras de compilação do grupo no fim do `makefile` do docente.

**Como verifiquei:** `make` sem avisos e `make test` sem falhas.
