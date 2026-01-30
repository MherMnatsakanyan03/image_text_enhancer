#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

static long long parse_ll(const char *s, long long def) {
    if (!s) return def;
    char *end = NULL;
    long long v = strtoll(s, &end, 10);
    return (end && *end == '\0') ? v : def;
}

int main(int argc, char **argv) {
    int threads = (int)parse_ll(argc > 1 ? argv[1] : NULL, omp_get_max_threads());
    long long ops_per_thread = parse_ll(argc > 2 ? argv[2] : NULL, 5LL * 1000 * 1000);
    int flush_iters = (int)parse_ll(argc > 3 ? argv[3] : NULL, 200000);

    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    printf("OpenMP shared-memory test\n");
    printf("  threads         = %d\n", threads);
    printf("  ops_per_thread  = %lld\n", ops_per_thread);
    printf("  flush_iters     = %d\n\n", flush_iters);

    // ------------------------------------------------------------
    // Test 1: Shared array visibility (write by each thread, barrier, read by all)
    // Barrier implies a flush, so all threads should see consistent values.
    // ------------------------------------------------------------
    int *shared = (int *)calloc((size_t)threads, sizeof(int));
    if (!shared) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    int failures = 0;
    #pragma omp parallel shared(shared, failures, threads)
    {
        int tid = omp_get_thread_num();
        shared[tid] = tid + 1;

        #pragma omp barrier

        int sum = 0;
        for (int i = 0; i < threads; i++) sum += shared[i];

        int expected = threads * (threads + 1) / 2;
        if (sum != expected) {
            #pragma omp atomic update
            failures++;
        }
    }

    printf("[Test 1] Shared array visibility: %s (failures=%d)\n",
           failures == 0 ? "PASS" : "FAIL", failures);

    // ------------------------------------------------------------
    // Test 2: Shared counter updates WITHOUT synchronization (race expected)
    // ------------------------------------------------------------
    long long counter = 0;
    #pragma omp parallel shared(counter, ops_per_thread)
    {
        for (long long i = 0; i < ops_per_thread; i++) {
            counter++; // data race on purpose
        }
    }

    long long expected = ops_per_thread * (long long)threads;
    printf("[Test 2] Counter without atomic: got=%lld expected=%lld %s\n",
           counter, expected, (counter == expected) ? "(surprisingly OK)" : "(RACE observed)");

    // ------------------------------------------------------------
    // Test 3: Shared counter updates WITH atomic (should match expected)
    // ------------------------------------------------------------
    counter = 0;
    #pragma omp parallel shared(counter, ops_per_thread)
    {
        for (long long i = 0; i < ops_per_thread; i++) {
            #pragma omp atomic update
            counter++;
        }
    }

    printf("[Test 3] Counter with atomic:    got=%lld expected=%lld %s\n",
           counter, expected, (counter == expected) ? "PASS" : "FAIL");

    // ------------------------------------------------------------
    // Test 4: Visibility/order using flush (two threads: writer/reader handshake)
    // Writer publishes data then flag; reader waits for flag and checks data.
    // ------------------------------------------------------------
    int data = 0;
    int flag = 0;
    int ok = 1;

    #pragma omp parallel num_threads(2) shared(data, flag, ok, flush_iters)
    {
        int tid = omp_get_thread_num();
        if (tid == 0) {
            for (int i = 1; i <= flush_iters; i++) {
                data = i;
                #pragma omp flush(data)
                flag = i;
                #pragma omp flush(flag)
            }
        } else {
            int last = 0;
            while (last < flush_iters) {
                #pragma omp flush(flag)
                int f = flag;
                if (f != last) {
                    #pragma omp flush(data)
                    if (data != f) {
                        ok = 0;
                        break;
                    }
                    last = f;
                }
            }
        }
    }

    printf("[Test 4] Flush publish/observe:  %s\n", ok ? "PASS" : "FAIL");

    free(shared);
    return (failures == 0 && ok) ? 0 : 2;
}
