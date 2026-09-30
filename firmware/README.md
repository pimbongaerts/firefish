# Firmware & software

Code to build for and operate the FireFish board.

## Contents

| Path | What it is | Status |
|---|---|---|
| `board-support/firefish_m4_v4b/` | Arduino board-support package (board definition, variant, linker scripts, package index) for **FireFish M4 v4b** | ✅ added (renamed from `_v2`) |
| `examples/firefish_m4_2025v2/` | Example implementation — a typical dive data-logging / intervalometer use case | ✅ added; ⚠️ pending code clean-up |

## Still to add / decide
- [x] **Bootloader** binary + self-updater added → `board-support/firefish_m4_v4b/bootloaders/firefish_m4/` (`v3.13.0-9-g2fd0593`); boards.txt reference fixed.
- [ ] Add **bootloader source** (uf2-samdx1 fork) — buildable source, not only the binary.
- [ ] Rebuild the Boards Manager distribution zip from the renamed source (see `board-support/.../README.md`).
- [x] License: **MIT** for original FireFish code (repo `LICENSE-SOFTWARE.txt`); third-party files keep upstream licenses (Adafruit core LGPL-2.1, uf2-samdx1 MIT, ChaN FatFs BSD-style) — see `LICENSE.md`.
- [ ] Finish the example clean-up (see its README).
