# GDEY029T94 p29 native-content probe + sourcing shortlist — 2026-09-21

No schematic, netlist, or footprint file changed this pass. This folder is
evidence only.

## 1. Native vector probe (deterministic, offline)

`hardware/kicad/tools/probe_p29_native_content.py` on the official
`datasheets/GDEY029T94.pdf` (SHA-256 `2ea22189…e876a9`, identity asserted in
the probe output). Raw output: `p29-native-content-probe.txt`.

Facts:

- p29 content stream is **1187 bytes**: 2 vector line segments, 0 curves,
  2 drawing objects (footer/rule), 0 circuit text labels — none of
  `PREVGH/PREVGL/PUMP/VSH1/VSH2/VCOM/1uF/4.7uF/47uH/D1/D2/D3` exist as text.
- The figure is the embedded 986×681 JPEG (xref 187, same image as the
  2026-09-19/20 raster passes); the 720×180 image (xref 263) and the sole
  Form XObject `KSPX93` (bbox 599×63 pt, bottom band) are footer furniture.
- **Stronger claim than prior passes:** not "untraceable at this raster
  resolution" but "structurally untraceable from this file by any parser at
  any fidelity" — there is no vector content and no text to associate.
  Re-probing this PDF for D2/D3 endpoints is closed.

## 2. Public-copy hunt (closed negative, 2026-09-21)

- `good-display.com` product page 465 and download page 1466: no
  machine-reachable SCH/reference-design file links (JS-rendered SPA).
- manuals.plus "PDF" URLs serve HTML interstitials, not PDFs.
- laskakit.cz mirror of the datasheet: **byte-identical** to the repo copy
  (`2ea22189…e876a9`, re-fetched and hashed today) — provenance
  confirmation only, same raster-only figure.
- No independent legible (vector or higher-resolution) copy of the §12
  figure found on any host checked.

## 3. MISSING_PART_IDENTITY sourcing shortlist (catalog evidence, NOT live)

For the next executor/reviewer. From the local jlcparts catalog snapshot
dated **2026-09-05** (`fetched_live=false`). These are candidate
Manufacturer/MPN values with LCSC part numbers — **not** live stock/price
evidence, and per repo policy symbol properties stay TBD until live
sourcing after the pump topology is fixed.

| Ref | Value (figure) | Candidate MPN | Manufacturer | LCSC | Tier | Note |
|-----|----------------|---------------|--------------|------|------|------|
| C5–C11, C14 | 1 µF 25 V 0402 | CL05A105KA5NQNC | Samsung Electro-Mechanics | C52923 | **Basic** | X5R, not the X7R the draft value assumes — dielectric deviation must be an explicit decision; closest basic 0402 1 µF/25 V in snapshot |
| C12 | 1 µF 25 V 0402 | CL05A105KA5NQNC | same | C52923 | Basic | same caveat |
| C13 | 4.7 µF 25 V 0603 | — | — | — | — | no basic 4.7 µF/25 V 0603 surfaced in snapshot search; needs extended-tier search |
| R5 | 100 Ω 0402 | — | — | — | — | value itself pending sensor-transient qualification (docs/schematic.md) |
| R7 | 1 MΩ 0402 | — | — | — | — | standard 1 % film; pick from the existing FOJAN FRC0402 family once topology fixed |
| R8 | 2.2 Ω 0402 | — | — | — | — | same |
| L1 | 47 µH 500 mA | — | — | — | — | needs saturation/DCR evidence vs panel pump current; figure gives rating only |
| J2/J3 | headers | — | — | — | — | J2 catalog record has no manufacturer/drawing (PLANNING_ONLY stands) |
| J4 | 24-pin 0.5 mm FPC | KH-FG0.5-H2.0-24PIN (datasheets/ on disk) | Kinghelm | TBD | TBD | drawing present in repo datasheets/; verify pin1 orientation vs panel §3 before adoption |

Caveat inherited from 2026-09-20: capacitor value/endpoint association to
figure refs C1–C12 is figure-family only until the edge list completes.

## 4. Verification actually run this pass

- `probe_p29_native_content.py`: exit 0, output above (PyMuPDF 1.28.2).
- No ERC/netlist regeneration needed (no source file touched); blocker gate,
  test_netlist, and PDF bounds were NOT re-run because nothing they check
  changed.
