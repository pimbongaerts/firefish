# FireFish M4 bootloader

UF2 bootloader for the FireFish M4 (SAMD51 / ATSAMD51J19A), based on Adafruit's
**uf2-samdx1** bootloader. Board volume when in bootloader mode: **FIREBOOT**.

**Bootloader version: `v3.13.0-9-g2fd0593`** — this is the version of the *bootloader
software*, independent of the board hardware revision (v0.4b). The two version numbers
are unrelated and both are correct.

## Files
- `bootloader-firefish_m4-v3.13.0-9-g2fd0593.bin` — flashable image (via SWD: J-Link + Microchip Studio, or OpenOCD).
- `bootloader-firefish_m4-v3.13.0-9-g2fd0593.elf` — same, with symbols (for debugging).
- `update-bootloader-firefish_m4-v3.13.0-9-g2fd0593.uf2` — self-update image: drag onto the FIREBOOT drive to update the bootloader from within itself (no programmer needed).
- `update-bootloader-firefish_m4-v3.13.0-9-g2fd0593.ino` — the self-updater as an Arduino sketch.
- `uf2_version.h` — `#define UF2_VERSION_BASE "v3.13.0-9-g2fd0593"`.

`boards.txt` references the `.bin` via `firefish_m4_v4b.bootloader.file` (fixed from the
previous stale `v3.10.0` path).

## TODO
- [ ] Add/link the **bootloader source** (uf2-samdx1 fork with the FireFish board config) — HardwareX wants buildable source, not only the binary.
- [ ] Note the Adafruit uf2-samdx1 upstream license (typically MIT) in `LICENSE-SOFTWARE`.
