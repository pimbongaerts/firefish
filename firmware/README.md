# Firmware & software

Everything needed to build for and operate the FireFish board (ATSAMD51J19A).

```
board-support/firefish_m4_v4b/   Arduino board-support package (board definition,
                                 pin variant, linker scripts, bootloader, package index)
examples/firefish_m4_2025v2/     Example sketch — dive data-logger + camera intervalometer
```

## Board support

FireFish's Arduino support is a **variant layered on the Adafruit SAMD core**, not a
standalone core — only the FireFish-specific files are included here. Board id
`firefish_m4_v4b`, display name **FireFish M4 v0.4b**.

1. In the Arduino IDE, install **Adafruit SAMD Boards core v1.7.10** (Boards Manager →
   "Adafruit SAMD Boards"). This variant was built against that version.
2. Copy the FireFish delta into the installed platform folder (on macOS, e.g.
   `~/Library/Arduino15/packages/adafruit/hardware/samd/1.7.10/`):
   - `boards.local.txt` → platform root (Arduino merges it with the core's `boards.txt`;
     **FireFish M4 v0.4b** then appears under *Tools ▸ Board*).
   - `variants/firefish_m4_v4b/` → the platform's `variants/` folder.
   - `bootloaders/firefish_m4/` → the platform's `bootloaders/` folder.

`package_firefish_index.json` is included for anyone who wants to publish a self-contained
Boards-Manager package instead (requires rebuilding a full-core zip and regenerating the
index checksum/size/url).

## Bootloader

UF2 bootloader based on Adafruit's **uf2-samdx1**, version `v3.13.0-9-g2fd0593` (the
bootloader-software version, independent of the board revision). In bootloader mode the
board enumerates as the **FIREBOOT** drive.

- `bootloaders/firefish_m4/*.bin` — flashable image (via SWD: J-Link + Microchip Studio, or OpenOCD).
- `*.uf2` / `*.ino` — self-updater: drag the `.uf2` onto the FIREBOOT drive to update the bootloader without a programmer.

**Building from source.** The only FireFish-specific files are the board definition in
`bootloaders/firefish_m4/source/` (`board_config.h` + `board.mk`); everything else comes
straight from upstream. Rebuild against the exact pinned commit (needs the ARM GCC toolchain):

```sh
git clone --recursive https://github.com/adafruit/uf2-samdx1
cd uf2-samdx1
git checkout 2fd0593            # = v3.13.0-9-g2fd0593
mkdir -p boards/firefish_m4
cp /path/to/source/board_config.h boards/firefish_m4/
cp /path/to/source/board.mk       boards/firefish_m4/
make BOARD=firefish_m4
```

## Example sketch

`examples/firefish_m4_2025v2/` is a typical use case: a real-time underwater data logger
and camera intervalometer. It reads the Bar30 depth sensor, Ping sonar altimeter, ADS1115
(LI-COR irradiance), RTC, and ADXL345 over I²C/SPI; shows live values on the ST7789 TFT;
triggers camera focus/shutter on an interval; and logs time-stamped records to a `data.csv`
on the onboard SPI flash (via ChaN **FatFs**). See the header comment in the `.ino` for
details, and the HardwareX article for operation.

Arduino library dependencies: Wire, SPI, Adafruit_GFX, Adafruit_ST7789, Adafruit_ADS1X15,
Adafruit_SPIFlash, Adafruit_I2CDevice, MS5837, RTClib, SparkFun_ADXL345, ping1d (Blue
Robotics), SdFat.

## Licensing

Original FireFish code is **MIT** (repo `LICENSE-SOFTWARE.txt`). Third-party files keep
their upstream licenses: Adafruit SAMD core / variant files → LGPL-2.1; uf2-samdx1
bootloader → MIT; ChaN FatFs (`ff.*`, `diskio.h`) → BSD-style. See `../LICENSE.md`.
