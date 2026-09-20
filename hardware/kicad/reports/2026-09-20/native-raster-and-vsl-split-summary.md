# GDEY029T94 p29 native-raster pass #2 + VSL/VGL split — 2026-09-20

Raw native-1:1 dumps in this folder (`native-diode-dumps.txt`,
`pins20-24-native-map.txt`, `left-pump-block-map.txt`, `region-trace.txt`);
this file is the interpretation record.

## What this pass added over 2026-09-19

1. **Native 1:1 extraction.** The p29 figure is one embedded JPEG (xref 187,
   986x681). This pass dumped it pixel-exact (no interpolation, threshold
   <140) at the three diode symbol areas, the pin-20..24 fan-out wedge, and
   the full left pump column. Deterministic, reproducible.
2. **White-region connectivity trace (deterministic topology).** Enclosed
   white regions of the ink layer partition connectivity: pin-22 and pin-23
   lead stubs sit on the boundary of *two different* enclosed regions
   (region 118 vs 119 at cap row; the pin rows' leftward stubs terminate in
   separate enclosed areas). The `PREVGL` text glyph and pin-23 stub share
   region 119; pin-22 has no label glyph in any shared region. This is
   region-topology corroboration (crossings without junctions partition the
   white background), **consistent with** the 2026-09-19 label-row finding —
   still not a wire-endpoint extraction, because the regions touch the
   unresolved pump wiring off-screen-left.
3. **Diode symbol geometry resolved — and it is uninformative.** At native
   resolution every one of D1/D2/D3 renders as a ~19x9 px filled bowtie:
   left edge vertical (x≈377-380), right edge converging point (x≈382→399),
   cathode bar as a short vertical segment merged into the triangle at the
   wide end. All three diodes are pixel-identical in shape (D1 y52-76,
   D2 y162-186, D3 y294-317), each on a continuous horizontal wire (D1 rows
   53-54 run 324/330-499; D2 rows 163-164 run x328-505+; D3 rows 295-296).
   The figure's *own* D1/D2/D3 are drawn with the same symbol orientation,
   yet their electrical roles differ (D1 is the positive-rail rectifier to
   PREVGH, D2/D3 the negative pump pair) — which proves symbol glyph
   orientation at this raster resolution cannot be used to infer net
   direction. **D2/D3 orientation therefore remains datasheet-figure
   UNTRACED.** The engineering-topology blockers stand unchanged.
4. **OCR re-probe of the pin-22/23 wedge** (8x LANCZOS crops) returned no
   readable label on the pin-22 row; the pin-21 row reads `1uF/` only —
   consistent with 2026-09-19.

## Split applied (bounded step from the 2026-09-19 plan)

Per GDEY029T94 Rev 1.0 §5 p8 (VSL pin 22 and VGL pin 23 are separate
capacitor pins of separate on-chip generators) plus the figure-label and
now region-topology corroboration, `PREVGL` was split in the editable
source generator:

- J4.22 (VSL) → new net `VSL` with its own 1uF/25V bypass `C14`
  (`Capacitor_SMD:C_0402_1005Metric`, footprint stays `TBD` placeholder via
  the generator's TBD-MPN rule), plus `PWR5` PWR_FLAG (ERC needs a declared
  source for the panel-derived rail, same convention as PWR3/PWR4).
- J4.23 (VGL) keeps `PREVGL` (C11, D3.K, PWR4 unchanged).
- `test_netlist.py` `CRITICAL_EXPECTED` J4.22 → `VSL` updated in the same
  change; on-sheet blocker text updated to record the split.
- `check_blockers.py` PANEL_RAIL_SHORT check retained as a regression guard
  (it now passes; the gate still exits 3 on the remaining groups).

Exported netlist membership after the split (verified):
`VSL = {C14.1, J4.22(VSL), PWR5}`; `PREVGL = {C11.1, D3.K, J4.23(VGL), PWR4}`.

## What is NOT resolved

- D2/D3 (and D4) lead-to-net endpoints: figure leads untraced; glyph
  orientation uninformative (point 3). The candidate's reversed-pump
  topology blockers remain, correctly.
- Full C5-C14 endpoint/value association against the figure; no new MPNs
  claimed — C14 carries figure-family value only (1uF/25V), identity TBD.
- Every MISSING_PART_IDENTITY / PLACEHOLDER_FOOTPRINT / DECLARED_UNVERIFIED
  finding (17/21/41 incl. the new C14) — unchanged by this pass.

## Verification actually run (2026-09-20, kicad-cli 9.0.9 native, local)

- `generate_schematic.py` regeneration + `git diff --exit-code`-clean re-run
- Native ERC: **0 violations** (`erc.json`, exit 0)
- `test_netlist.py`: pass + negative mutation rejected
- `test_schematic_bounds.py`: pass
- `test_blocker_gate.py`: 10/10 OK (rejected/passing fixtures both correct)
- PDF export + `test_pdf_bounds.py`: pass
- `check_blockers.py` on fresh export: **exit 3, 5 groups** (was 6 —
  PANEL_RAIL_SHORT cleared): NEGATIVE_PUMP_CLAMP, NEGATIVE_PUMP_RECTIFIER,
  MISSING_PART_IDENTITY(17), PLACEHOLDER_FOOTPRINT(21),
  DECLARED_UNVERIFIED(41). Electrical approval NOT claimed.
