/* Unit tests for Core/Src/debounce.c */

#include <string.h>

#include "debounce.h"
#include "test_util.h"

#define DEBOUNCE_SAMPLES 5  /* BUTTONS_DEBOUNCE_MS in buttons.c, one read per ms */

/*
 * Feeds reads ('1' = pressed, '0' = released, one per ms) and compares the
 * reported edges with `expected`, e.g. "P9 R24" = press at 9 ms, release at 24 ms.
 */
static void check_edges(const char *name, const char *reads, const char *expected)
{
    debounce_t d;
    char got[128] = "";
    char edge[16];

    debounce_init(&d, DEBOUNCE_SAMPLES);
    for (int t = 0; reads[t] != '\0'; t++) {
        if (debounce_update(&d, reads[t] == '1')) {
            snprintf(edge, sizeof edge, "%s%c%d", got[0] ? " " : "", d.stable ? 'P' : 'R', t);
            strncat(got, edge, sizeof got - strlen(got) - 1);
        }
    }
    CHECK(strcmp(got, expected) == 0, "%s: got \"%s\", expected \"%s\"", name, got, expected);
}

int main(void)
{
    check_edges("clean press and release",
                "000001111111111111110000000000", "P9 R24");
    check_edges("bouncy press and release",
                "00000101101111111111111010010000000000", "P14 R32");
    check_edges("2 ms noise spike ignored",
                "000000110000000000", "");
    check_edges("4 ms spike, just below threshold, ignored",
                "000001111000000000", "");
    check_edges("two quick 10 ms taps",
                "00000111111111100000000001111111111000000000", "P9 R19 P29 R39");
    return TEST_RESULT();
}
