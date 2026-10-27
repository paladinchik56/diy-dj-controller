/* Unit tests for Core/Src/analog_filter.c */

#include "analog_filter.h"
#include "test_util.h"

#define SCAN_PERIOD_MS  4   /* analog_scan() period in app_tasks.c */

static uint16_t clamp_raw(int v)
{
    return v < 0 ? 0 : v > 4095 ? 4095 : (uint16_t)v;
}

/* A control resting on a 7-bit step boundary must not chatter */
static void test_resting_7bit(void)
{
    analog_filter_t f;
    uint16_t out;
    int changes = 0;

    analog_filter_init(&f, 127, 8);
    int boot_changes = 0;
    for (int i = 0; i < 17; i++) {      /* first reading + settling phase */
        boot_changes += analog_filter_update(&f, clamp_raw(2048 + test_noise(6)), &out);
    }
    CHECK(boot_changes <= 2, "power-up gave %d reports, expected at most 2", boot_changes);

    for (int trial = 0; trial < 20; trial++) {  /* 20 x 10 s at 250 Hz */
        for (int i = 0; i < 2500; i++) {
            changes += analog_filter_update(&f, clamp_raw(2048 + test_noise(6)), &out);
        }
    }
    CHECK(changes == 0, "resting 7-bit pot changed %d times in 200 s", changes);
}

/* A slow sweep must be monotonic and end exactly at the top */
static void test_sweep_7bit(void)
{
    analog_filter_t f;
    uint16_t out;
    int last = -1, distinct = 0;

    analog_filter_init(&f, 127, 8);
    for (int i = 0; i <= 1000; i++) {
        if (analog_filter_update(&f, clamp_raw(i * 4095 / 1000 + test_noise(6)), &out)) {
            CHECK((int)out > last, "sweep went backwards: %d after %d", out, last);
            last = out;
            distinct++;
        }
    }
    for (int i = 0; i < 50; i++) {
        analog_filter_update(&f, clamp_raw(4095 + test_noise(6)), &out);
    }
    CHECK(f.out == 127, "sweep ended at %u, expected 127", f.out);
    CHECK(distinct >= 100, "sweep reported only %d distinct values", distinct);
}

/* Pots that stop short of the rails must still reach 0 and the maximum */
static void test_end_dead_zones(void)
{
    analog_filter_t f;
    uint16_t out;

    analog_filter_init(&f, 127, 8);
    for (int i = 0; i < 100; i++) {
        analog_filter_update(&f, clamp_raw(15 + test_noise(3)), &out);
    }
    CHECK(f.out == 0, "low end gave %u, expected 0", f.out);

    for (int i = 0; i < 100; i++) {
        analog_filter_update(&f, clamp_raw(4080 + test_noise(3)), &out);
    }
    CHECK(f.out == 127, "high end gave %u, expected 127", f.out);
}

/* 14-bit pitch: quiet at rest, fine steps when moving, full range */
static void test_pitch_14bit(void)
{
    analog_filter_t f;
    uint16_t out;
    int changes = 0;

    analog_filter_init(&f, 16383, 6);
    for (int i = 0; i < 17; i++) {      /* first reading + settling phase */
        analog_filter_update(&f, clamp_raw(1500 + test_noise(4)), &out);
    }
    for (int i = 0; i < 2500; i++) {
        changes += analog_filter_update(&f, clamp_raw(1500 + test_noise(4)), &out);
    }
    CHECK(changes == 0, "resting pitch fader changed %d times", changes);

    int steps = 0;
    analog_filter_init(&f, 16383, 6);
    for (int i = 0; i <= 4000; i++) {
        steps += analog_filter_update(&f, clamp_raw(i * 4095 / 4000 + test_noise(4)), &out);
    }
    for (int i = 0; i < 50; i++) {
        analog_filter_update(&f, 4095, &out);
    }
    CHECK(steps > 500, "pitch sweep gave only %d steps", steps);
    CHECK(f.out == 16383, "pitch sweep ended at %u, expected 16383", f.out);
}

/* A full-range jump (crossfader cut) must be followed within one scan */
static void test_fast_move(void)
{
    analog_filter_t f;
    uint16_t out;
    int scans = 0;

    analog_filter_init(&f, 127, 8);
    analog_filter_update(&f, 0, &out);
    while (f.out != 127 && scans < 1000) {
        analog_filter_update(&f, 4095, &out);
        scans++;
    }
    CHECK(scans == 1, "0 -> max took %d scans (%d ms), expected 1",
          scans, scans * SCAN_PERIOD_MS);
}

int main(void)
{
    test_resting_7bit();
    test_sweep_7bit();
    test_end_dead_zones();
    test_pitch_14bit();
    test_fast_move();
    return TEST_RESULT();
}
