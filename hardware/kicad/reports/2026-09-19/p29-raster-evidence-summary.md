# GDEY029T94 §12 p29 reference-circuit raster evidence pass — 2026-09-19

Raw probe dumps in this folder; this is the interpretation record. Nothing
here completes the outstanding edge-by-edge trace; it narrows it.

## Method (reproducible)

- Source: `datasheets/GDEY029T94.pdf`, Rev 1.0 (SHA-256
  `2ea221896f9b0463e7f53b6a4d0dff967728f2049ca7667d45878c1ff4e876a9`,
  matches the manifest).
- Page 29 contains only headings in its text layer; the figure is a single
  JPEG (xref 187, 986x681). No vector paths, no embedded text.
- Rendered the page at 8x with PyMuPDF (4760x6737) and ran RapidOCR
  (det+rec, no local tesseract available) for label placement; then probed
  raw dark-pixel run geometry through symbol rows (see the `-probes`/
  `-region-probe` files).

## What the raster evidence deterministically shows

1. Label inventory of the figure (OCR conf ≥0.9 unless noted):
   D1/D2/D3 all `MBR0530`; Q1 `Si1308EDL`; L1 `47uH 500mA`;
   `4.7uF/25V` at C4; `1uF/25V` at C5, C6, C9, C10, C11, C12;
   `R1 >1M`, `R2 2.2` (Ω); `3.3V` rail entering from the left;
   `FPC 24PIN(0.5mm)` connector.
2. FPC row association (pin-number column x≈452 pt):
   internal net label `PREVGH` sits on the row of pin **21** (`VGH`),
   `PREVGL` on the row of pin **23** (`VGL`), `VCOM` on pin **24**.
   Pin **22** carries only the function-column label `VSL`
   (`gd-p29-ocr-labels.txt` rows y≈374..401). No internal `PREVGL`-style
   rail label was detected on the pin-22 row. The raw slice probe of that
   region (`gd-p29-pins20-24-region-probe.txt`) shows each of the pin
   20–24 rows carrying its own leftward lead stub into the RC column;
   whether the pin-22 and pin-23 leads merge further left is
   indeterminate at native resolution (crossing/junction ambiguity). The
   deterministic observation is the label-row association above.
3. This is consistent with the text layer of the same datasheet: §5 p8
   lists pin 22 `VSL` (C-type, negative **source** driving voltage) and
   pin 23 `VGL` (C-type, supply for negative gate/VCOM/VSL generation)
   as **separate capacitor pins**, each driven by the on-chip generators.
   Wiring both to one net (current candidate: J4.22=J4.23=`PREVGL`) pits
   two generator outputs against each other. The `PANEL_RAIL_SHORT`
   blocker therefore stands with datasheet-text + figure-label evidence
   (not merely reviewer reasoning).

## What it does NOT show

- Wire-level endpoint connectivity. At native 986x681 raster resolution,
  crossings cannot be distinguished from junctions by morphological
  probing; D1/D2/D3 lead-to-net endpoints were **not** extracted, so the
  D2/D3 orientation blockers remain as previously recorded (engineering
  topology confidence, not a manufacturer-diagram trace).
- The exact C5-C12 reservoir/flying-capacitor endpoint map.
- Any component rating or pad-numbering validation.

## Cross-reference note (not identity evidence)

The Solomon Systech SSD1680 controller datasheet (public mirrors fetched
2026-09-19, hashes in
[datasheet manifest](../../../docs/datasheet-sources.md))
describes 24 pins whose names/types match GDEY029T94 §5 one-for-one and
publishes a matching application-circuit figure family (Fig 13-1, with a
text component table: C0-C1,C2-C7 1uF, C8 0.47/1uF @ >0.25uF DC-bias,
R1 2.2 ohm, D1-D3 MBR0530-class, Q1 Si1304BDL/NX3008NBK-class, L1 47uH).
Per `docs/component-selection.md` caveat 4, GDEY029T94's own DS does not
name its driver IC, so this may **not** be used to claim the panel IS
SSD1680, nor to substitute its figure for the panel figure. It is
recorded only as a same-family consistency signal and as the only
vector-quality version of the pump figure available; the authoritative
resolution must still trace GDEY029T94 §12 p29 itself.

## Sharpened remaining task for issue #3

One bounded trace: enumerate the p29 edges D1/D2/D3/L1/C13-equivalent
flying cap between `3.3V`/switch node/two negative rails, and give
J4.22 (VSL) its own net + capacitor, then update `test_netlist.py`
`CRITICAL_EXPECTED`, the `check_blockers.py` expected-pass fixtures, and
the on-sheet blocker text together. Values/identities in item 1 above can
then populate C5-C13/L1 from the figure plus real sourcing.
