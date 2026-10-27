# Breadboard test setup

Diagram: [`breadboard.svg`](breadboard.svg). Pin map: `PINOUT.md`.
Flashing: `FLASHING.md`.

Build and check in stages, so a problem always points at the last thing added.

## Stage 1: board + ST-Link, LED blinks

1. Put the Blue Pill across the breadboard centre gap. Both BOOT jumpers on 0.
2. ST-Link → SWD header: SWDIO→DIO, SWCLK→CLK, GND→GND, 3.3V→3V3.
3. Flash (`FLASHING.md`).
4. Expected: the PC13 LED blinks at 1 Hz (= running, USB not connected).

## Stage 2: USB-MIDI

1. Remove the ST-Link 3.3V wire (keep SWDIO, SWCLK, GND).
2. Connect the Blue Pill micro-USB to the laptop with a micro-USB to USB-C
   cable (a data cable, not charge-only).
3. Expected: LED steady on; `aseqdump -l` lists "DIY DJ Controller".
4. If the device does not appear: `dmesg -w` while plugging in. Some Blue Pills
   have a wrong 10k pull-up (R10) on D+ instead of 1.5k; many PCs still accept
   it, otherwise put a 1.8k resistor between PA12 and 3.3V.

## Stage 3: debug UART (optional)

USB-UART adapter set to 3.3 V: adapter RXD → PA9, TXD → PA10, GND → GND. Do not
connect its VCC/5V. Open it at 115200 8N1:
```bash
picocom -b 115200 /dev/ttyUSB0
```
Expected after reset: `dj controller boot, SYSCLK = 72000000 Hz`.

## Stage 4: buttons

1. Power rails: Blue Pill 3.3 → red rail, G → blue/black rail. Link the top and
   bottom rail pairs. Long breadboards often split each rail in the middle.
2. Each button: one leg to its pin, the other to GND. No resistor.
   6x6 buttons have 4 legs; use two diagonally opposite legs.
3. Expected in `aseqdump -p "DIY DJ Controller"`: Note On / Note Off per press
   (table in `MIDI_MAP.md`).

## Stage 5: multiplexers and knobs

1. Mux breakout: VCC → +3V3 (never 5 V), GND → GND, EN → GND, S0..S3 →
   PB12..PB15 (both mux boards share these four wires), SIG → PA0 (MUX A) or
   PA1 (MUX B). 100 nF ceramic between VCC and GND close to each board.
2. Each knob / fader: outer legs to +3V3 and GND, middle leg (wiper) to its mux
   channel. If a control works backwards, swap the outer legs.
3. Start with MUX B and one fader, then add the rest.
4. Expected: Control Change messages while moving, none while untouched.
   Channels without a pot float and may produce random values; that is normal
   until a pot is connected there.

## Pins that stay free

PA8, PA15, PB0, PB1 (spare); PA11/PA12 (USB, on-board); 5V pin; PC14/PC15/VB.
