# FireFish M4 v0.4b — Arduino board support

FireFish's Arduino support is a **variant layered on the Adafruit SAMD core**, not a
standalone core. Only the FireFish-specific files live here; everything else (the C++
core, compiler toolchain, USB stacks, etc.) comes from the Adafruit SAMD core.

Board id: `firefish_m4_v4b` · display name: **FireFish M4 v0.4b** · MCU: ATSAMD51J19A.

## Contents (the FireFish delta)
```
boards.local.txt                 ← the FireFish M4 v0.4b board definition (only)
variants/firefish_m4_v4b/        ← FireFish pin map + linker scripts
  pins_arduino.h  variant.cpp  variant.h  linker_scripts/
bootloaders/firefish_m4/         ← UF2 bootloader binary + self-updater (see its README)
package_firefish_index.json      ← optional: for a self-contained Boards-Manager package
```

## Dependency
- **Adafruit SAMD Boards core, v1.7.10** (the version this variant was built against).
  Install it in the Arduino IDE first (Boards Manager → "Adafruit SAMD Boards").

## Install (manual overlay — recommended)
After installing the Adafruit SAMD core, copy the FireFish delta into that installed
platform folder (path like
`~/Library/Arduino15/packages/adafruit/hardware/samd/1.7.10/` on macOS):
1. `boards.local.txt` → the platform root (Arduino merges it with the core's `boards.txt`; **"FireFish M4 v0.4b"** then appears in *Tools ▸ Board*).
2. `variants/firefish_m4_v4b/` → the platform's `variants/` folder.
3. `bootloaders/firefish_m4/` → the platform's `bootloaders/` folder.

`boards.local.txt` only declares the FireFish board; the shared menus (Cache, CPU Speed,
Optimize, …) come from the Adafruit `boards.txt`.

## Optional: self-contained Boards-Manager package
`package_firefish_index.json` describes a self-contained platform installable via a
Boards Manager URL. That path requires **rebuilding a full-core zip** (Adafruit core +
this delta) and regenerating the index `checksum`/`size`/`url`. Not needed for the manual
overlay above; kept here for whoever publishes the Boards-Manager package.

## TODO
- [x] Bootloader reference fixed (`v3.13.0-9-g2fd0593`; stale `v3.10.0` removed).
- [ ] Confirm the exact Adafruit core version if different from 1.7.10.
- [ ] (Optional) build + host the self-contained Boards-Manager package.
