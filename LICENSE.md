# Licensing

FireFish is open-source hardware. Following the OSHWA guidance, its parts are
licensed separately:

| Part | License | File |
|---|---|---|
| **Hardware** — KiCad schematic & PCB, Gerbers, enclosure STLs, BOM | **CERN-OHL-S v2** (`CERN-OHL-S-2.0`) | [`LICENSE-HARDWARE.txt`](LICENSE-HARDWARE.txt) |
| **Software / firmware** — original code (example sketch logic, board definition) | **MIT** | [`LICENSE-SOFTWARE.txt`](LICENSE-SOFTWARE.txt) |
| **Documentation** — build docs, figures, wiring diagram | **CC BY 4.0** (`CC-BY-4.0`) | [`LICENSE-DOCS.txt`](LICENSE-DOCS.txt) |

Copyright © 2024–2026 Pim Bongaerts.

## Third-party components (retain their upstream licenses)
Some firmware files are derived from or bundled with third-party open-source
projects and are **not** relicensed — they keep their original licenses:

- `firmware/board-support/firefish_m4_v4b/` — `boards.local.txt`, `variants/…`
  (`variant.cpp`, `variant.h`, `pins_arduino.h`, linker scripts) are derived from the
  **Adafruit SAMD core** / Arduino core → **LGPL-2.1** (see file headers).
- `firmware/…/bootloaders/firefish_m4/` — **Adafruit uf2-samdx1** bootloader → **MIT**.
- `firmware/examples/firefish_m4_2025v2/ff.c, ff.h, ffconf.h, diskio.h` — **ChaN FatFs**
  → its own BSD-style permissive license (see file headers).

The **MIT** license above applies only to the original FireFish code, not to these
third-party files.

## How to apply / cite
- Keep this notice and the per-file headers intact in redistributions.
- Under CERN-OHL-S, modified hardware design files must be shared under the same license.
- Under CC BY / MIT, retain attribution to Pim Bongaerts and the FireFish HardwareX article.
