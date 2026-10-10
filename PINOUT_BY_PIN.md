# Pinout — by STM32 pin

Platform: STM32F103C8T6 ("Blue Pill", LQFP48 package). Columns: **Pin** → **Connection**
(direct wire, or which multiplexer + channel) → **Peripheral** (the STM32 function used)
→ **Panel function** (what it's responsible for on the controller).

Device-centric version: `PINOUT.md`. Visual version: `schematic.svg`.

## Port A

| Pin | Connection | Peripheral | Panel function |
|---|---|---|---|
| PA0 | wire to MUX A `SIG` (CD74HC4067, channels C0–C12) | ADC1, ADC12_IN0 | reads whichever of the 13 potentiometers is currently addressed: Master, Gain/Bass/Mid/Treble/FX1/FX2 × Deck A, Gain/Bass/Mid/Treble/FX1/FX2 × Deck B |
| PA1 | wire to MUX B `SIG` (CD74HC4067, channels C0–C4) | ADC1, ADC12_IN1 | reads whichever of the 5 sliders is currently addressed: Pitch A, Pitch B, Channel fader A, Channel fader B, Crossfader |
| PA2 | direct wire to button | GPIO input, internal pull-up | Play — Deck A |
| PA3 | direct wire to button | GPIO input, internal pull-up | Cue — Deck A |
| PA4 | direct wire to button | GPIO input, internal pull-up | Sync — Deck A |
| PA5 | direct wire to button | GPIO input, internal pull-up | Shift |
| PA6 | direct wire to button | GPIO input, internal pull-up | Play — Deck B |
| PA7 | direct wire to button | GPIO input, internal pull-up | Cue — Deck B |
| PA8 | free | — (reserved, unused) | none — free for future use |
| PA9 | wire to USB-UART adapter (debug only, optional) | USART1 TX | debug/serial prints in parallel with USB-MIDI — not a panel control |
| PA10 | wire to USB-UART adapter (debug only, optional) | USART1 RX | debug/serial prints in parallel with USB-MIDI — not a panel control |
| PA11 | hard-wired, on-board | USB D- | link to host computer (USB-MIDI) — not an individual panel control |
| PA12 | hard-wired, on-board | USB D+ | link to host computer (USB-MIDI) — not an individual panel control |
| PA13 | wired to ST-Link header | SWDIO | firmware flashing / debugging — not a panel control |
| PA14 | wired to ST-Link header | SWCLK | firmware flashing / debugging — not a panel control |
| PA15 | free | — (reserved, unused) | none — free for future use |

## Port B

| Pin | Connection | Peripheral | Panel function |
|---|---|---|---|
| PB0 | no wire | ADC1, ADC12_IN8 (unused) | spare — left free on purpose for future analog controls |
| PB1 | no wire | ADC1, ADC12_IN9 (unused) | spare — left free on purpose for future analog controls |
| PB2 | no wire (hard-pulled to GND on most Blue Pill boards) | BOOT1 | boot-mode select — not usable as GPIO |
| PB3 | direct wire to button | GPIO input, internal pull-up — **needs JTAG-disable remap at startup** (defaults to JTDO at reset) | Sync — Deck B |
| PB4 | direct wire to button | GPIO input, internal pull-up — **needs JTAG-disable remap at startup** (defaults to JNTRST at reset) | Hotcue 1 — Deck A |
| PB5 | direct wire to button | GPIO input, internal pull-up | Hotcue 2 — Deck A |
| PB6 | direct wire to button | GPIO input, internal pull-up | Hotcue 3 — Deck A |
| PB7 | direct wire to button | GPIO input, internal pull-up | Hotcue 4 — Deck A |
| PB8 | direct wire to button | GPIO input, internal pull-up | Hotcue 1 — Deck B |
| PB9 | direct wire to button | GPIO input, internal pull-up | Hotcue 2 — Deck B |
| PB10 | direct wire to button | GPIO input, internal pull-up | Hotcue 3 — Deck B |
| PB11 | direct wire to button | GPIO input, internal pull-up | Hotcue 4 — Deck B |
| PB12 | wire to S0 on both MUX A and MUX B (shared address bus) | GPIO output | mux channel-select, bit 0 — not a panel control itself, but selects which pot/slider PA0/PA1 are currently reading |
| PB13 | wire to S1 on both MUX A and MUX B (shared address bus) | GPIO output | mux channel-select, bit 1 |
| PB14 | wire to S2 on both MUX A and MUX B (shared address bus) | GPIO output | mux channel-select, bit 2 |
| PB15 | wire to S3 on both MUX A and MUX B (shared address bus) | GPIO output | mux channel-select, bit 3 |

## Port C / D / other

| Pin | Connection | Peripheral | Panel function |
|---|---|---|---|
| PC13 | on-board, not routed to the panel | GPIO (3 mA limit) | built-in Blue Pill LED — not a panel control |
| PC14, PC15 | not broken out on this package | RTC crystal (unused) | none |
| PD0 | on-board, soldered to crystal | OSC_IN | 8 MHz clock source — not a panel control |
| PD1 | on-board, soldered to crystal | OSC_OUT | 8 MHz clock source — not a panel control |
| NRST | wired to reset button (on-board) | reset | board reset — not a panel control |
| BOOT0 | on-board jumper/pin | boot-mode select | not GPIO, not a panel control |

## Notes

- **MUX A / MUX B** are both CD74HC4067 16-channel analog multiplexers, sharing the
  same 4-bit address bus (PB12–PB15: S0–S3). The firmware steps through addresses
  0–15 and reads PA0 (pots) and PA1 (sliders) at each step — this is how 18 analog
  controls are read through only 2 ADC pins.
- Each potentiometer/slider has 3 leads: `+3V3` and `GND` go to the shared power bus
  on the perfboard, only the wiper goes to its MUX channel pin — so "connection" for
  PA0/PA1 above refers to the signal path only, not the power wiring.
- Each button has 2 leads: one to GND, one to its GPIO — no external resistor, the
  firmware enables the pin's internal pull-up.
- PB3/PB4 default to JTAG functions (JTDO/JNTRST) at reset. Before using them as
  button GPIOs, the firmware must disable the JTAG-only part of the debug port while
  keeping SW-DP (STM32Cube HAL: `__HAL_AFIO_REMAP_SWJCFG_JTAGDISABLE()`), done once
  early in `main()`.
