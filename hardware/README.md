# Hardware — FireFish PCB

Canonical **KiCad 8** design for the FireFish board (ATSAMD51J19A), plus the fabrication
outputs used for the PCBWay build. Licensed **CERN-OHL-S v2** (see repo `LICENSE.md`).

## Schematic

![FireFish schematic](../docs/images/schematic.png)

> ⚠️ **Placeholder render.** This PNG was exported from an earlier revision (rev v01, KiCad 5.1.5)
> and is included for a quick overview only. The authoritative, current schematic is
> `firefish.kicad_sch` (KiCad 8, rev v0.4b); a fresh export will replace this image.

## Contents
```
firefish.kicad_pro            KiCad 8 project
firefish.kicad_sch            schematic (symbols embedded)
firefish.kicad_pcb            2-layer PCB layout (footprints embedded)
firefish-rescue.kicad_sym     rescued symbols (referenced by the schematic)
sym-lib-table / fp-lib-table  project library tables (cleaned — see note)
firefish_BOM_pcbway2024.csv   authoritative BOM (matches the assembled board)
gerbers/                      fabrication: F/B copper, mask, paste, silkscreen,
                              Edge_Cuts, + PTH/NPTH drill files (2024-08-23)
placement/PCBWay_positions.csv  pick-and-place / component positions (2024-08-23)
```

## Opening the project
The schematic and PCB are **self-contained**: KiCad 8 embeds all used symbols
(`lib_symbols`) and footprints, so the project opens, renders, and can be re-plotted
without any external libraries.

## Note on library tables
The original `sym-lib-table` / `fp-lib-table` referenced libraries by **absolute local
paths** (e.g. the Digi-Key KiCad library and custom footprint folders on the author's
machine) that would be broken for anyone else. Those entries were removed for this
deposit; only the project-local `firefish-rescue` symbol library is retained. This does
not affect viewing, plotting, or fabrication — only *editing a placed part from its
original source library* would require re-adding that library locally. Custom footprints
used include the Digi-Key KiCad library, an Open Book (`OSO-BOOK-A1-06`) footprint, and a
custom Molex 52271-2069 footprint.

## Fabrication summary
- 2-layer PCB. Gerbers + drills as fabricated by PCBWay (2024-08-23).
- BOM cross-checked against the layout: all 93 placed components accounted for; JP1/JP2
  are open solder-bridge jumpers (not populated). See manuscript §4 / repo BOM.

## TODO
- [ ] Replace the placeholder schematic PNG with a fresh render from the current KiCad 8 design (rev v0.4b).
- [ ] Regenerate **ERC** on this KiCad 8 design (the archived `.erc` is stale: 2021 / KiCad 5).
- [ ] (Optional) bundle the custom footprint `.pretty` libraries for full editability.
