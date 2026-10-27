/*
 * Whole-firmware host simulation scenarios (see sim/sim.h).
 *
 *   sim_controller <scenario> [-v]
 *
 * -v prints every MIDI message the simulated host received. "demo" always
 * prints its log; it is a walkthrough, not a test.
 */

#include <stdio.h>
#include <string.h>

#include "sim.h"
#include "test_util.h"

static bool verbose;

/* Latest a button message may arrive: bounce + 5 ms debounce + 1 ms scan + 1 USB frame */
#define BUTTON_LATENCY_MAX_MS   8U

/* ------------------------------------------------------------------------ helpers */

static void connect_and_settle(void)
{
    sim_run_ms(100);
    sim_usb_connect();
    sim_run_ms(100);
}

/* Host value equals the firmware output, and is close to the ideal position */
static void check_analog_synced(const sim_analog_t *a, uint16_t raw)
{
    const uint16_t host = sim_host_value(a);
    const uint16_t fw = sim_fw_output(a);
    const uint16_t ideal = sim_ideal_value(a, raw);
    const int tol = a->high_res ? 40 : 1;
    const int diff = (int)host - (int)ideal;

    CHECK(host == fw, "%s: host has %u, firmware output is %u", a->name, host, fw);
    CHECK(diff >= -tol && diff <= tol, "%s: host has %u, ideal for raw %u is %u",
          a->name, host, raw, ideal);
}

/* Index of the first message from `from` on matching status and data1, or -1 */
static long find_msg(size_t from, uint8_t status, uint8_t data1)
{
    for (size_t i = from; i < sim_log_count(); i++) {
        const sim_msg_t *m = sim_log_at(i);
        if (m->bytes[0] == status && m->bytes[1] == data1) {
            return (long)i;
        }
    }
    return -1;
}

static size_t count_msg(size_t from, uint8_t status, uint8_t data1)
{
    size_t n = 0;
    for (size_t i = from; i < sim_log_count(); i++) {
        const sim_msg_t *m = sim_log_at(i);
        n += m->bytes[0] == status && m->bytes[1] == data1;
    }
    return n;
}

static void check_clean_stream(void)
{
    CHECK(sim_host_protocol_errors() == 0, "%u malformed USB-MIDI packets", sim_host_protocol_errors());
    CHECK(sim_host_double_note_on() == 0, "%u Note On for an already held note", sim_host_double_note_on());
    CHECK(sim_host_orphan_note_off() == 0, "%u Note Off without Note On", sim_host_orphan_note_off());
}

/* ------------------------------------------------------------------------ scenarios */

/* Every button and control reaches the host on the channel and number from MIDI_MAP.md */
static void scenario_mapping(void)
{
    sim_boot();
    connect_and_settle();

    for (size_t i = 0; i < sim_button_count; i++) {
        const sim_button_t *b = &sim_buttons[i];
        const size_t from = sim_log_count();

        sim_button(b->name, true, 2);
        sim_run_ms(20);
        sim_button(b->name, false, 2);
        sim_run_ms(20);

        CHECK(sim_log_count() - from == 2, "%s: %zu messages, expected Note On + Note Off",
              b->name, sim_log_count() - from);
        CHECK(find_msg(from, (uint8_t)(0x90 | b->midi_ch), b->note) >= 0, "%s: no Note On ch%u note %u",
              b->name, b->midi_ch + 1U, b->note);
        CHECK(find_msg(from, (uint8_t)(0x80 | b->midi_ch), b->note) >= 0, "%s: no Note Off ch%u note %u",
              b->name, b->midi_ch + 1U, b->note);
    }

    for (size_t i = 0; i < sim_analog_count; i++) {
        const sim_analog_t *a = &sim_analogs[i];
        const size_t from = sim_log_count();
        const uint16_t raw = (uint16_t)(600 + 150 * i);

        sim_analog(a->name, raw);
        sim_run_ms(40);

        /* Only this control may have sent anything */
        for (size_t k = from; k < sim_log_count(); k++) {
            const sim_msg_t *m = sim_log_at(k);
            const bool mine = m->bytes[0] == (0xB0 | a->midi_ch) &&
                              (m->bytes[1] == a->cc || (a->high_res && m->bytes[1] == a->cc + 32));
            CHECK(mine, "moving %s sent %02X %02X %02X", a->name, m->bytes[0], m->bytes[1], m->bytes[2]);
        }
        check_analog_synced(a, raw);
    }
    check_clean_stream();
}

/* Positions set before the cable is plugged in reach the host right after mount */
static void scenario_boot_sync(void)
{
    sim_boot();
    uint16_t raw[32];
    for (size_t i = 0; i < sim_analog_count; i++) {
        raw[i] = (uint16_t)((i * 977U + 300U) % 4096U);
        sim_analog(sim_analogs[i].name, raw[i]);
    }

    sim_run_ms(300);
    CHECK(sim_log_count() == 0, "%zu messages before USB was connected", sim_log_count());

    const uint32_t t_mount = sim_now();
    sim_usb_connect();
    sim_run_ms(20);

    for (size_t i = 0; i < sim_analog_count; i++) {
        check_analog_synced(&sim_analogs[i], raw[i]);
    }
    /* 18 controls, the two pitch faders send 2 messages each: 20 messages, more
     * than the 16-packet FIFO. A pitch MSB that fits while its LSB does not
     * is sent again with the LSB on the next scan, so allow those repeats. */
    CHECK(sim_log_count() >= sim_analog_count + 2 && sim_log_count() <= sim_analog_count + 4,
          "%zu messages for the initial sync, expected %zu..%zu",
          sim_log_count(), sim_analog_count + 2, sim_analog_count + 4);
    CHECK(sim_log_at(sim_log_count() - 1)->t_rx_ms - t_mount <= 10, "initial sync took %u ms",
          sim_log_at(sim_log_count() - 1)->t_rx_ms - t_mount);
    check_clean_stream();
}

/* Untouched controls with ADC noise must stay silent */
static void scenario_idle_noise(void)
{
    sim_boot();
    sim_set_noise(6);
    for (size_t i = 0; i < sim_analog_count; i++) {
        sim_analog(sim_analogs[i].name, (uint16_t)(250 + 210 * i));
    }
    connect_and_settle();

    const uint32_t t0 = sim_now();
    sim_run_ms(60000);
    CHECK(sim_messages_since(t0) == 0, "%zu messages in 60 s with nothing touched", sim_messages_since(t0));
    check_clean_stream();
}

/* Four buttons with bouncing contacts while all faders and four knobs move at once */
static void scenario_mash(void)
{
    static const char *const buttons[] = { "PLAY_A", "CUE_B", "HC1_A", "HC4_B" };
    static const char *const moving[] = { "PITCH_A", "PITCH_B", "CHANNEL_A", "CHANNEL_B", "CROSSFADER",
                                          "GAIN_A", "BASS_B", "FX1_A", "MASTER" };
    uint16_t target[sizeof moving / sizeof moving[0]];

    sim_boot();
    for (size_t i = 0; i < sizeof moving / sizeof moving[0]; i++) {
        sim_analog(moving[i], 0);
        target[i] = 4095;
    }
    connect_and_settle();
    const size_t from = sim_log_count();
    const uint32_t t_press = sim_now();

    for (size_t i = 0; i < sizeof moving / sizeof moving[0]; i++) {
        sim_analog_ramp(moving[i], target[i], 300);
    }
    for (size_t i = 0; i < 4; i++) {
        sim_button(buttons[i], true, 3);
    }
    sim_run_ms(60);
    const uint32_t t_release = sim_now();
    for (size_t i = 0; i < 4; i++) {
        sim_button(buttons[i], false, 3);
    }
    sim_run_ms(400);

    for (size_t i = 0; i < 4; i++) {
        const sim_button_t *b = sim_find_button(buttons[i]);
        const uint8_t on = (uint8_t)(0x90 | b->midi_ch), off = (uint8_t)(0x80 | b->midi_ch);
        const long i_on = find_msg(from, on, b->note), i_off = find_msg(from, off, b->note);

        CHECK(count_msg(from, on, b->note) == 1, "%s: %zu Note On", b->name, count_msg(from, on, b->note));
        CHECK(count_msg(from, off, b->note) == 1, "%s: %zu Note Off", b->name, count_msg(from, off, b->note));
        if (i_on >= 0) {
            const uint32_t lat = sim_log_at((size_t)i_on)->t_rx_ms - t_press;
            CHECK(lat <= BUTTON_LATENCY_MAX_MS, "%s: Note On after %u ms", b->name, lat);
        }
        if (i_off >= 0) {
            const uint32_t lat = sim_log_at((size_t)i_off)->t_rx_ms - t_release;
            CHECK(lat <= BUTTON_LATENCY_MAX_MS, "%s: Note Off after %u ms", b->name, lat);
        }
    }
    for (size_t i = 0; i < sizeof moving / sizeof moving[0]; i++) {
        check_analog_synced(sim_find_analog(moving[i]), target[i]);
    }
    CHECK(sim_host_notes_held() == 0, "%u notes still held at the end", sim_host_notes_held());
    check_clean_stream();
}

/* Releasing Shift before the button still releases the shifted note */
static void scenario_shift(void)
{
    sim_boot();
    connect_and_settle();
    const size_t from = sim_log_count();

    sim_button("SHIFT", true, 2);   sim_run_ms(20);
    sim_button("PLAY_A", true, 2);  sim_run_ms(20);
    sim_button("SHIFT", false, 2);  sim_run_ms(20);
    sim_button("PLAY_A", false, 2); sim_run_ms(20);
    sim_button("PLAY_A", true, 2);  sim_run_ms(20);
    sim_button("PLAY_A", false, 2); sim_run_ms(20);

    static const uint8_t expected[][2] = {
        { 0x92, 0 },    /* Shift on                    */
        { 0x90, 64 },   /* Play A, shifted note        */
        { 0x82, 0 },    /* Shift off                   */
        { 0x80, 64 },   /* Play A off, still shifted   */
        { 0x90, 0 },    /* Play A without Shift        */
        { 0x80, 0 },
    };
    const size_t n = sizeof expected / sizeof expected[0];

    CHECK(sim_log_count() - from == n, "%zu messages, expected %zu", sim_log_count() - from, n);
    for (size_t k = 0; k < n && from + k < sim_log_count(); k++) {
        const sim_msg_t *m = sim_log_at(from + k);
        CHECK(m->bytes[0] == expected[k][0] && m->bytes[1] == expected[k][1],
              "message %zu is %02X %02X, expected %02X %02X",
              k, m->bytes[0], m->bytes[1], expected[k][0], expected[k][1]);
    }
    CHECK(sim_host_notes_held() == 0, "%u notes still held", sim_host_notes_held());
    check_clean_stream();
}

/* Faders moved while unplugged are sent again after re-plugging; stale
 * button presses from that time are not */
static void scenario_reconnect(void)
{
    sim_boot();
    connect_and_settle();

    sim_usb_disconnect();
    sim_analog("CROSSFADER", 4000);
    sim_analog("PITCH_A", 1000);
    sim_button("PLAY_A", true, 2);      /* pressed and released while unplugged */
    sim_run_ms(100);
    sim_button("PLAY_A", false, 2);
    sim_run_ms(100);

    const size_t from = sim_log_count();
    sim_usb_connect();
    sim_run_ms(20);
    CHECK(find_msg(from, 0x90, 0) < 0 && find_msg(from, 0x80, 0) < 0,
          "a button pressed while unplugged was sent after re-plugging");
    for (size_t i = 0; i < sim_analog_count; i++) {
        const sim_analog_t *a = &sim_analogs[i];
        const uint16_t raw = strcmp(a->name, "CROSSFADER") == 0 ? 4000
                           : strcmp(a->name, "PITCH_A") == 0 ? 1000 : 2047;
        check_analog_synced(a, raw);
    }
    check_clean_stream();
}

/*
 * Congested host: it collects one USB-MIDI packet every 2 ms (a busy
 * computer), while every control moves at once and buttons are pressed.
 * The FIFO fills up; nothing may get corrupted or lost.
 */
static void scenario_slow_host(void)
{
    static const char *const buttons[] = { "PLAY_A", "CUE_A", "PLAY_B", "SYNC_B" };

    sim_boot();
    connect_and_settle();
    sim_host_rate(1, 2);
    const size_t from = sim_log_count();

    for (size_t i = 0; i < sim_analog_count; i++) {
        sim_analog_ramp(sim_analogs[i].name, 4095, 200);
    }
    sim_run_ms(50);
    for (size_t i = 0; i < 4; i++) {
        sim_button(buttons[i], true, 2);
    }
    sim_run_ms(40);
    for (size_t i = 0; i < 4; i++) {
        sim_button(buttons[i], false, 2);
    }
    sim_run_ms(2000);

    CHECK(sim_fifo_full_events() > 0, "scenario did not fill the FIFO, it tests nothing");
    for (size_t i = 0; i < 4; i++) {
        const sim_button_t *b = sim_find_button(buttons[i]);
        CHECK(count_msg(from, (uint8_t)(0x90 | b->midi_ch), b->note) == 1, "%s: Note On lost", b->name);
        CHECK(count_msg(from, (uint8_t)(0x80 | b->midi_ch), b->note) == 1, "%s: Note Off lost", b->name);
    }
    for (size_t i = 0; i < sim_analog_count; i++) {
        check_analog_synced(&sim_analogs[i], 4095);
    }
    CHECK(sim_host_notes_held() == 0, "%u notes stuck on", sim_host_notes_held());
    check_clean_stream();
}

/* Walkthrough with the full log: plug in, press, shift, move faders */
static void scenario_demo(void)
{
    verbose = true;
    sim_boot();
    sim_analog("CROSSFADER", 0);
    sim_analog("PITCH_A", 3000);

    printf("-- t=100 ms: USB cable plugged in, the panel state is sent once\n");
    sim_run_ms(100);
    sim_usb_connect();
    sim_run_ms(50);
    sim_print_log(0);

    size_t from = sim_log_count();
    printf("-- Play A pressed with a bouncing contact (3 ms), released 100 ms later\n");
    sim_button("PLAY_A", true, 3);
    sim_run_ms(100);
    sim_button("PLAY_A", false, 3);
    sim_run_ms(50);
    sim_print_log(from);

    from = sim_log_count();
    printf("-- Shift held, Cue B pressed and released\n");
    sim_button("SHIFT", true, 2);
    sim_run_ms(30);
    sim_button("CUE_B", true, 2);
    sim_run_ms(30);
    sim_button("CUE_B", false, 2);
    sim_run_ms(30);
    sim_button("SHIFT", false, 2);
    sim_run_ms(30);
    sim_print_log(from);

    from = sim_log_count();
    printf("-- Crossfader swept left to right in 150 ms while Hotcue 1 A is tapped\n");
    sim_analog_ramp("CROSSFADER", 4095, 150);
    sim_run_ms(60);
    sim_button("HC1_A", true, 2);
    sim_run_ms(40);
    sim_button("HC1_A", false, 2);
    sim_run_ms(100);
    sim_print_log(from);

    from = sim_log_count();
    printf("-- Pitch A nudged slightly (14-bit: MSB + LSB)\n");
    sim_analog_ramp("PITCH_A", 3010, 20);
    sim_run_ms(60);
    sim_print_log(from);
}

/* ------------------------------------------------------------------------ main */

typedef struct {
    const char *name;
    void (*run)(void);
} scenario_t;

static const scenario_t scenarios[] = {
    { "mapping",    scenario_mapping },
    { "boot_sync",  scenario_boot_sync },
    { "idle_noise", scenario_idle_noise },
    { "mash",       scenario_mash },
    { "shift",      scenario_shift },
    { "reconnect",  scenario_reconnect },
    { "slow_host",  scenario_slow_host },
    { "demo",       scenario_demo },
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s <scenario> [-v]\nscenarios:", argv[0]);
        for (size_t i = 0; i < sizeof scenarios / sizeof scenarios[0]; i++) {
            printf(" %s", scenarios[i].name);
        }
        printf("\n");
        return 2;
    }
    verbose = argc > 2 && strcmp(argv[2], "-v") == 0;

    for (size_t i = 0; i < sizeof scenarios / sizeof scenarios[0]; i++) {
        if (strcmp(argv[1], scenarios[i].name) == 0) {
            scenarios[i].run();
            if (verbose && strcmp(argv[1], "demo") != 0) {
                sim_print_log(0);
            }
            printf("scenario %s: %s\n", argv[1], test_failures ? "FAILED" : "passed");
            return test_failures ? 1 : 0;
        }
    }
    printf("unknown scenario %s\n", argv[1]);
    return 2;
}
