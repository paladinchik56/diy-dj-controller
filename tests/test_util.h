#ifndef DJ_TEST_UTIL_H
#define DJ_TEST_UTIL_H

/*
 * Minimal test helpers: CHECK() reports a failure with file and line but keeps
 * running, TEST_RESULT() turns the failure count into the process exit code
 * that ctest looks at (0 = pass).
 */

#include <stdint.h>
#include <stdio.h>

static int test_failures;

#define CHECK(cond, ...)                                                  \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                   \
            printf(__VA_ARGS__);                                          \
            printf("\n");                                                 \
            test_failures++;                                              \
        }                                                                 \
    } while (0)

#define TEST_RESULT()                                                     \
    (printf("%s: %s\n", __FILE__, test_failures ? "FAILED" : "passed"),   \
     test_failures ? 1 : 0)

/* Deterministic pseudo-random noise in [-amp, +amp], same on every machine */
static uint32_t test_rng_state = 12345U;

static inline int test_noise(int amp)
{
    test_rng_state = test_rng_state * 1103515245U + 12345U;
    return (int)((test_rng_state >> 16) % (uint32_t)(2 * amp + 1)) - amp;
}

#endif //DJ_TEST_UTIL_H
