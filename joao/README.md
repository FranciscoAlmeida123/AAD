# AAD A1 — SHA1 a acabar em 32 bits a zero

Programas que alteram ficheiros PDF, PNG, PPM, HTML e TXT para que o `sha1sum` acabe em
`00000000`. Enunciado: `AAD_A1.pdf` (eLearning). Os ficheiros do docente estão intocados na pasta acima (`Projeto1/`).

## Compilar e testar (Linux / WSL2)

```bash
python3 scripts/make_inputs.py   # gera inputs/ (só se faltarem)
make                             # bin/aad_modify (+ testes)
make test                        # testes de formatos/núcleo + testes da CLI
make portable                    # só o motor escalar (Raspberry Pi, telemóvel)
```

## Utilização

```
aad_modify [-e scalar|avx|avx2|avx512|omp|cuda] [-j threads] [-t segundos] [-m] input output
```

- O tipo deduz-se da extensão do input: `.pdf .png .ppm .html/.htm .txt`.
- `-m` (modo de medição): corre `-t` segundos e escreve a linha CSV
  `engine,threads,file_type,n_blocks,seconds,block_updates,candidates,solutions`.
- Código de saída: 0 encontrou e escreveu; 1 erro de ficheiro; 2 mal chamado; 3 sem solução no tempo dado.

Medir um núcleo: `taskset -c 0 ./bin/aad_modify -m -t 60 -e scalar inputs/aad_a1.pdf /tmp/x.pdf`

## Onde se mexe em cada ficheiro

| Tipo | Alteração | Bytes variáveis |
|---|---|---|
| PDF  | `%`+enchimento+11 var+`\n` logo antes do último `startxref` | 11 carateres `?`..`~` |
| PNG  | chunk `tEXt` (`Comment`) logo antes do `IEND`, com o CRC ajustado | 11 carateres |
| PPM  | LSB dos últimos 48 bytes de cor (16 píxeis); lido com `read_ppm_file` do docente (cabeçalho `P6 W H 255`, sem comentários) | 48 bits |
| HTML | `<!--`+enchimento+11 var+`-->` no fim | 11 carateres |
| TXT  | última linha: enchimento+11 var | 11 carateres |

O enchimento (0–63 bytes) faz com que o tamanho final tenha L mod 64 = 55. Assim só o último
bloco SHA1 muda por tentativa, e o estado dos blocos anteriores (o midstate) calcula-se uma vez.

## Estrutura

`src/` código (`aad_files.c` formatos e template, `aad_mutate.h` contador→candidato,
`aad_scalar.c` motor de referência, `aad_modify.c` CLI) · `tests/` · `scripts/` · `inputs/` ·
`outputs/` (ficheiros finais) · `results/` · `docs/ia/` (registo do uso de IA).
