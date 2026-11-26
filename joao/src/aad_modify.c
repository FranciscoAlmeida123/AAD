//
// aad_modify.c --- modifies a file so that its SHA1 ends with 32 zero bits
//
//   aad_modify [-e scalar|avx|avx2|avx512|omp|cuda] [-j threads] [-t seconds] [-m] input output
//
// -m (measurement): searches for -t seconds (60 by default, setup excluded), does not stop at the
// first solution, and prints one CSV line:
//   engine,threads,file_type,n_blocks,seconds,block_updates,candidates,solutions
// Exit: 0 = found and written; 1 = file error; 2 = bad usage; 3 = no solution in the given time.
//

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "aad_engines.h"
#include "aad_files.h"
#include "aad_utilities.h" // teacher: time_measurement, wall_time_delta

#define MAX_FOUND 64u
#define BATCH     (1u << 20) // counters per engine call; the clock is checked between calls

typedef enum { E_SCALAR, E_AVX, E_AVX2, E_AVX512, E_OMP, E_CUDA } engine_t;
static const char *const engine_names[] = { "scalar", "avx", "avx2", "avx512", "omp", "cuda" };

static void usage(FILE *fp)
{
    fprintf(fp,
            "usage: aad_modify [-e scalar|avx|avx2|avx512|omp|cuda] [-j threads] [-t seconds] [-m] input output\n"
            "  -e  search engine (default scalar)\n"
            "  -j  threads for -e omp (default: all online CPUs)\n"
            "  -t  time limit in seconds (default 60)\n"
            "  -m  measurement mode: run -t seconds and print a CSV line\n"
            "      engine,threads,file_type,n_blocks,seconds,block_updates,candidates,solutions\n"
            "  input type from the extension: .pdf .png .ppm .html/.htm .txt\n"
            "exit: 0 found and written, 1 file error, 2 bad usage, 3 no solution in time\n");
}

static int engine_available(engine_t e)
{
#if defined(AAD_PORTABLE)
    return e == E_SCALAR;
#else
    switch (e) {
        case E_AVX:    return __builtin_cpu_supports("avx");
        case E_AVX2:   return __builtin_cpu_supports("avx2");
        case E_AVX512: return __builtin_cpu_supports("avx512f");
        default:       return 1;
    }
#endif
}

static u64_t run_engine(engine_t e, const search_template_t *t, u64_t first, u64_t count, u64_t *found, u32_t *n_found,
                        int threads)
{
#if defined(AAD_PORTABLE)
    (void)e;
    (void)threads;
    return search_scalar(t, first, count, found, MAX_FOUND, n_found);
#else
    switch (e) {
        case E_AVX:    return search_avx(t, first, count, found, MAX_FOUND, n_found);
        case E_AVX2:   return search_avx2(t, first, count, found, MAX_FOUND, n_found);
        case E_AVX512: return search_avx512(t, first, count, found, MAX_FOUND, n_found);
        case E_OMP:    return search_omp(t, first, count, found, MAX_FOUND, n_found, threads);
        case E_CUDA:   return search_cuda(t, first, count, found, MAX_FOUND, n_found);
        default:       return search_scalar(t, first, count, found, MAX_FOUND, n_found);
    }
#endif
}

int main(int argc, char **argv)
{
    engine_t engine = E_SCALAR;
    int threads = 0, measure = 0, opt, k;
    double seconds = 60.0;
    char *end;
    file_type_t type;
    search_template_t t;

    while ((opt = getopt(argc, argv, "e:j:t:mh")) != -1) {
        switch (opt) {
            case 'e':
                for (k = 0; k < 6 && strcmp(optarg, engine_names[k]) != 0; k++)
                    ;
                if (k == 6) { fprintf(stderr, "aad_modify: unknown engine \"%s\"\n", optarg); return 2; }
                engine = (engine_t)k;
                break;
            case 'j': threads = (int)strtol(optarg, &end, 10); if (*end || threads < 1) { fprintf(stderr, "aad_modify: bad -j\n"); return 2; } break;
            case 't': seconds = strtod(optarg, &end); if (*end || !(seconds > 0.0)) { fprintf(stderr, "aad_modify: bad -t\n"); return 2; } break;
            case 'm': measure = 1; break;
            case 'h': usage(stdout); return 0;
            default:  usage(stderr); return 2;
        }
    }
    if (argc - optind != 2) { usage(stderr); return 2; }
    const char *in_path = argv[optind], *out_path = argv[optind + 1];
    if (file_type_from_path(in_path, &type) != 0) {
        fprintf(stderr, "aad_modify: unknown file type of \"%s\"\n", in_path);
        return 2;
    }
    if (!engine_available(engine)) {
        fprintf(stderr, "aad_modify: engine \"%s\" not available on this build/CPU\n", engine_names[engine]);
        return 2;
    }
    if (engine == E_OMP && threads == 0)
        threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (engine != E_OMP)
        threads = 1;
    if (make_template(in_path, type, &t) != 0)
        return 1;
#if !defined(AAD_PORTABLE)
    if (engine == E_CUDA && cuda_init(&t) != 0) {
        fprintf(stderr, "aad_modify: cuda_init failed\n");
        return 1;
    }
#endif

    // search in fixed batches until the time is over (or, without -m, until the first solution)
    u64_t found[MAX_FOUND], counter = 0, updates = 0, solutions = 0, winner = 0;
    int have_winner = 0;
    double elapsed = 0.0;
    time_measurement();
    while (elapsed < seconds && (measure || !have_winner)) {
        u32_t n_found = 0;
        updates += run_engine(engine, &t, counter, BATCH, found, &n_found, threads);
        if (n_found > 0 && !have_winner) {
            winner = found[0];
            have_winner = 1;
        }
        solutions += n_found;
        counter += BATCH;
        time_measurement();
        elapsed += wall_time_delta(); // wall time of this batch
    }
#if !defined(AAD_PORTABLE)
    if (engine == E_CUDA)
        cuda_done();
#endif
    if (measure)
        printf("%s,%d,%s,%u,%.3f,%lu,%lu,%lu\n", engine_names[engine], threads, file_type_name(t.type), t.n_blocks, elapsed,
               updates, counter, solutions);
    if (!have_winner) {
        if (!measure)
            fprintf(stderr, "aad_modify: no solution in %.1f s (%lu candidates)\n", elapsed, counter);
        return 3;
    }
    if (write_result(in_path, &t, winner, out_path) != 0)
        return 1;
    if (!measure)
        fprintf(stderr, "aad_modify: counter %lu, %.2f s, %lu candidates -> %s\n", winner, elapsed, counter, out_path);
    return 0;
}
