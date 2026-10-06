# DIY USB-MIDI DJ Controller

Pet project: a custom DJ controller built around an STM32F103C8T6 "Blue Pill" board, programmed with STM32CubeIDE/CubeMX. Native USB only — the board enumerates as a USB-MIDI class device, so DJ software (VirtualDJ, Serato, Traktor, Mixxx, ...) binds directly to its buttons/knobs/faders via MIDI Learn, with no separate driver or bridge app needed. Audio itself stays on the connected computer; the controller has no audio I/O of its own.

Status: parts ordered, wiring verified against the official datasheet, firmware not started yet. Case/enclosure is a separate, later task.

## Design

- 13 rotary potentiometers (gain/bass/mid/treble/FX x2 per deck, plus master)
- 5 slide potentiometers (2x pitch, 2x channel fader, crossfader)
- 15 momentary buttons (play/cue/sync/hotcues/shift per deck)
- Only 10 of the Blue Pill's pins are ADC-capable (PA0-PA7, PB0, PB1 — confirmed against the ST datasheet in this project), so the 18 analog controls are read through two CD74HC4067 multiplexers (one for the pots, one for the sliders) sharing a single 4-wire address bus, leaving most ADC channels free for future expansion.
- All controls connect to an intermediate perfboard through pin headers + Dupont connectors rather than being soldered directly, so any part can be swapped without a soldering iron.

See [`schematic.svg`](schematic.svg) for the wiring diagram and [`PINOUT.md`](PINOUT.md) for the full device → pin table.

## Bill of materials (Amazon.de)

| # | Part | Qty needed | Link | Note |
|---|---|---|---|---|
| 1 | STM32F103C8T6 "Blue Pill" | 1 | [single board](https://www.amazon.de/Entwicklungsplatine-STM32F103-STM32F103C8T6-MicroUSB-kompatibel/dp/B0D64RXF8Z) or [5-pack](https://www.amazon.de/TECNOIOT-STM32F103C8T6-Minimum-Development-Arduino/dp/B07WSDYNQG) | spares are cheap insurance for a hand-soldered DIY board |
| 2 | CD74HC4067 16-ch multiplexer | 2 | [3-pack](https://www.amazon.de/-/en/CD74HC4067-16-Channel-Digital-Multiplexer-Breakout/dp/B06Y1L95GK) | one for the pots, one for the sliders |
| 3 | Rotary potentiometer, 10k ohm linear (knob included) | 13 | [10-pack, Lewttyer](https://www.amazon.de/Drehpotentiometer-Potentiometer-Potenziometer-Unterlegscheiben-Lewttyer/dp/B093DRBG3X) | get 2 packs (20, with spares); knobs are already bundled, no separate knob purchase needed |
| 4 | Slide potentiometer, 10k ohm linear, 75mm travel | 5 | [5-pack, Fyearfly B10K](https://www.amazon.de/Schiebepotentiometer-Zweikanaliger-gerader-Schiebemischer-mehrere/dp/B09PBXB47T) | quantity matches exactly |
| 5 | Momentary push button, 6x6x4.3mm | 15 | [100-pack, Gazechimp](https://www.amazon.de/Gazechimp-Stk-Drucktaster-Schalter-Drucktaste-6x6x4-3mm/dp/B01N5FZZ34) | bulk pack, cheaper with spares |
| 6 | Slider caps | 5 | [20-pack, universal 4mm](https://www.amazon.de/sourcing-map-Schieberegler-Potentiometer-24mmx11mmx10mm/dp/B0DTH8TVGB) | check shaft width against the Fyearfly sliders before relying on fit |
| 7 | Perfboard, FR1, 9x15cm, 48x30 holes, pre-tinned | 1 (of a 10-pack) | [10-pack, sourcing-map](https://www.amazon.de/sourcing-map-Einseitige-Leiterplatte-Lochrasterplatine/dp/B07H3QNYMC) | confirmed size: fits both mux chips, power rails, ~23 MCU-side pins plus ~33 control leads |
| 8 | Wire, 26AWG silicone, 6 colors x 9m | 1 | [link](https://www.amazon.de/75mm%C2%B2Silikon-Elektronik-Elektrischer-RC-Flugzeuge-Haushaltsger%C3%A4te/dp/B09XWCZ5KC) | silicone insulation stays flexible through repeated rewiring |
| 9 | Brass standoffs + M3 screws/nuts, assorted | 1 | [150-piece set](https://www.amazon.de/Messing-Abstands-Unterst%C3%BCtzung-Distanzschraube-Sortiment/dp/B01CN75XGI) | covers Blue Pill mounting now and case mounting later |
| 10 | Micro-USB male/female panel-mount extension, 50cm | 1 | [link](https://www.amazon.de/Verl%C3%A4ngerungskabel-Micro-USB-Stecker-Buchse-Verl%C3%A4ngerung-Schrauben/dp/B07DS6KBJX) | screw-mount bulkhead connector; confirmed to work in series with a USB-C to Micro-USB cable to the laptop, as long as both actually carry USB data lines (not charge-only) |
| 11 | Ceramic capacitor, 100nF / 0.1uF, 50V | 1 (of a 10-pack) | [10-pack](https://www.amazon.de/Keramikkondensator-Keramik-Kondensator-100nF-St%C3%BCck/dp/B0DNXZ2386) | decoupling cap at each multiplexer's VCC/GND pins, need 2 minimum |
| 12 | 2.54mm male pin headers + male-female Dupont jumper wires | as needed | generic, any source | soldered headers on the perfboard plus Dupont female sockets replace the JST-XH connectors originally planned — same swap-without-soldering goal, no crimping tool needed |

### Still open

- Enclosure/case — deferred on purpose, next task.
- Indicator LEDs at the Play buttons (present in the original reference design) — not currently in the pin budget; add later if wanted.
- Shaft/stem compatibility between the sourced slider caps and the Fyearfly sliders, and between the Lewttyer potentiometer knobs and their shaft — worth double-checking against Amazon reviews/Q&A before ordering, since listing photos aren't always reliable.

## Firmware

- **STM32CubeIDE** — IDE used for editing, building, flashing and debugging the firmware
- **STM32CubeMX** — clock tree and peripheral configuration, generates the HAL-based project skeleton
- **STM32Cube HAL** (Hardware Abstraction Layer) — GPIO, ADC and USART peripheral drivers
- **STM32Cube USB Device middleware (USB_DEVICE)** — the base USB stack that the custom class sits on top of
- **USB-MIDI class** (MIDI Streaming, a subclass of USB Audio Class 1.0) — CubeMX has no turnkey template for this on the F1 family, so it will be adapted from ST's own official MIDI streaming example (originally shipped for F4 Discovery boards) rather than generated from scratch
- **C** — the language STM32Cube projects are generated in and built with
- **ADC** (STM32F103's 12-bit ADC1, 2 channels used: PA0 for MUX A's `SIG`, PA1 for MUX B's `SIG`) — scanning the two multiplexers across their 16 channels each
- **GPIO** — digital inputs with internal pull-up for the 15 buttons; digital outputs for the shared S0–S3 multiplexer address lines
- **USART1** — debug/serial prints running in parallel with USB-MIDI
- **SWD via ST-Link** (PA13/PA14) — flashing and debugging; requires disabling the JTAG-only part of the debug port early in `main()` (HAL: `__HAL_AFIO_REMAP_SWJCFG_JTAGDISABLE()`) so PB3/PB4 are freed from their reset-default JTDO/JNTRST function and usable as plain button GPIOs
- No RTOS planned — bare-metal/superloop firmware
