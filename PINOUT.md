# Pinout — device → signal path → STM32 pin

Platform: STM32F103C8T6 ("Blue Pill", LQFP48 package — confirmed against ST datasheet
DS5319: only PA0–PA7, PB0, PB1 are ADC-capable on this package, so anything beyond
10 analog channels must go through a multiplexer).

See `schematic.svg` for the visual version of this table.

## Analog — potentiometers (13×, through MUX A)

| Device | Path | STM32 pin |
|---|---|---|
| Master | MUX A (CD74HC4067), channel C0 | PA0 (ADC12_IN0, via MUX A `SIG`) |
| Gain — Deck A | MUX A, C1 | PA0 |
| Bass — Deck A | MUX A, C2 | PA0 |
| Mid — Deck A | MUX A, C3 | PA0 |
| Treble — Deck A | MUX A, C4 | PA0 |
| FX1 — Deck A | MUX A, C5 | PA0 |
| FX2 — Deck A | MUX A, C6 | PA0 |
| Gain — Deck B | MUX A, C7 | PA0 |
| Bass — Deck B | MUX A, C8 | PA0 |
| Mid — Deck B | MUX A, C9 | PA0 |
| Treble — Deck B | MUX A, C10 | PA0 |
| FX1 — Deck B | MUX A, C11 | PA0 |
| FX2 — Deck B | MUX A, C12 | PA0 |

Each potentiometer has 3 leads: `+3V3` and `GND` go to the shared power bus on the
perfboard (not individually to the MCU), the wiper goes to its MUX A channel pin.

## Analog — sliders (5×, through MUX B)

| Device | Path | STM32 pin |
|---|---|---|
| Pitch — Deck A | MUX B (CD74HC4067), channel C0 | PA1 (ADC12_IN1, via MUX B `SIG`) |
| Pitch — Deck B | MUX B, C1 | PA1 |
| Channel fader — Deck A | MUX B, C2 | PA1 |
| Channel fader — Deck B | MUX B, C3 | PA1 |
| Crossfader | MUX B, C4 | PA1 |

Same 3-lead rule as the potentiometers: power/ground on the shared bus, only the
wiper is individually wired, into its MUX B channel.

## Multiplexer control (shared between MUX A and MUX B)

| Signal | Path | STM32 pin |
|---|---|---|
| S0 | address bus, both chips | PB12 |
| S1 | address bus, both chips | PB13 |
| S2 | address bus, both chips | PB14 |
| S3 | address bus, both chips | PB15 |
| EN (both chips) | tied straight to GND on the perfboard | — (not wired to the MCU) |
| VCC (both chips) | +3.3V bus; **100nF ceramic decoupling capacitor** between VCC and GND at each chip, as close to the chip as possible | — (fed from Blue Pill `3V3` pin) |
| GND (both chips) | GND bus | — |

## Buttons (15×, direct GPIO, no multiplexer, no external resistor)

Each button: one leg to the GND bus, the other leg to its own GPIO. Firmware enables
the internal pull-up, so the pin reads high when idle and low when pressed — no
external resistor needed.

| Device | Path | STM32 pin | Note |
|---|---|---|---|
| Play — Deck A | direct | PA2 | — |
| Cue — Deck A | direct | PA3 | — |
| Sync — Deck A | direct | PA4 | — |
| Shift | direct | PA5 | — |
| Play — Deck B | direct | PA6 | — |
| Cue — Deck B | direct | PA7 | — |
| Sync — Deck B | direct | PB3 | ⚠ defaults to JTDO (JTAG) at reset — needs JTAG-disable remap before use as GPIO |
| Hotcue 1 — Deck A | direct | PB4 | ⚠ defaults to JNTRST (JTAG) at reset — needs JTAG-disable remap before use as GPIO |
| Hotcue 2 — Deck A | direct | PB5 | — |
| Hotcue 3 — Deck A | direct | PB6 | — |
| Hotcue 4 — Deck A | direct | PB7 | — |
| Hotcue 1 — Deck B | direct | PB8 | — |
| Hotcue 2 — Deck B | direct | PB9 | — |
| Hotcue 3 — Deck B | direct | PB10 | — |
| Hotcue 4 — Deck B | direct | PB11 | — |

## Reserved — do not use for controls

| Pin(s) | Reserved for | Why |
|---|---|---|
| PA8, PA9, PA10 | USART1 (debug UART) | kept free for serial debug prints alongside USB-MIDI |
| PA11, PA12 | USB D-/D+ | native USB-MIDI, hard-wired function |
| PA13, PA14 | SWDIO/SWCLK | ST-Link programming/debug |
| PD0, PD1 | OSC_IN/OSC_OUT | 8 MHz crystal is physically soldered here |
| NRST, BOOT0 | reset / boot-mode select | not GPIO at all |
| PB2 (BOOT1) | boot-mode select | on most Blue Pill boards this is already hard pulled to GND on the PCB |
| PC13 | onboard LED | shared with the board's built-in LED, 3 mA sink/source limit |
| PB0, PB1 | — | left free on purpose as spare ADC channels (ADC12_IN8/9) for future additions |
