# Flashing and debugging

Programmer: ST-Link V2 over SWD. Wiring: see `breadboard.svg` (right side of the
Blue Pill) and `BREADBOARD.md`.

| ST-Link pin | Blue Pill SWD header |
|---|---|
| SWDIO | DIO |
| SWCLK | CLK (DCLK) |
| GND | GND |
| 3.3V | 3V3, **only** when the Blue Pill is not powered from its own USB cable |

Both BOOT jumpers stay on 0.

## Option 1: flash only, with STM32CubeProgrammer (works right away)

The build defines a `flash` target when `STM32_Programmer_CLI` is installed
(found in `~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin`).

- CLion: pick the **flash** target in the run/target selector and press Build
  (Ctrl+F9). It builds the firmware first, then writes, verifies and resets.
- Terminal:
  ```bash
  cmake --build --preset Debug --target flash
  ```

## Option 2: flash and debug in CLion, with OpenOCD

1. Install OpenOCD once:
   ```bash
   sudo apt install openocd
   ```
2. CLion: Settings → Build, Execution, Deployment → Embedded Development →
   OpenOCD Location: `/usr/bin/openocd` (press Test).
3. Run configuration **Flash and Debug (OpenOCD)** (stored in
   `.idea/runConfigurations/`, local only). If it is missing, create it:
   Run → Edit Configurations → + → OpenOCD Download & Run; Target and
   Executable `dj`; Board config file `openocd.cfg` from the project root;
   Download: Updated only; Reset: Init.
4. Run (Shift+F10) flashes and starts the firmware; Debug (Shift+F9) flashes
   and stops at `main()` with breakpoints, variables and stepping.
5. Peripheral registers in the debugger: Peripherals tab → Load SVD →
   `~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/SVD/STM32F103.svd`.

Manual flash from the terminal:
```bash
openocd -f openocd.cfg -c "program build/Debug/dj.elf verify reset exit"
```

## Troubleshooting

- `UNEXPECTED idcode` / `Cannot identify target`: clone chip; `openocd.cfg`
  already sets `CPUTAPID 0`. For STM32CubeProgrammer, update it, or use OpenOCD.
- `Error: init mode failed` / `No device found`: check the three SWD wires and
  GND, and that the board is powered (red power LED on the Blue Pill).
- Board cannot be reached after a bad firmware: hold the RESET button, start
  flashing, release RESET right after (or set BOOT0 = 1 for one flash).
- `LIBUSB_ERROR_ACCESS`: missing udev rules for ST-Link. They are installed
  with STM32CubeProgrammer (`/etc/udev/rules.d/49-stlinkv2.rules`); replug the
  ST-Link after installing.
