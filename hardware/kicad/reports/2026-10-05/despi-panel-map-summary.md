# DESPI-C02 rail-map comparison (2026-10-05)

## Source and method

`tools/probe_despi_panel_net_map.py` fetches Good Display's standalone
[DESPI-C02_SCH V1.0.pdf](https://www.good-display.com/companyfile/DESPI-C02-SCH-30.html)
and fails unless the PDF SHA-256 equals
`b1893766d212249429876aa1cb3e7cf929712ac2f2b262ce2da39948c26128b1`.
It reads vector glyphs and the PDF text grid (not raster OCR or conductor
connectivity). The unmodified JSON and assertion output from this run are in
`despi-panel-map.json` and `despi-panel-map-checks.txt`. All assertions passed.
An earlier naive value association misattributed a nearby capacitor value to
D2; the checked-in probe matches values on the component's row before nearby
rows. Its assertions are a regression check on this hash-pinned reference,
not an independent electrical proof.

## Provenance result

The DESPI-C02 **reference board is not an exact copy** of the GDEY029T94
panel's §12 circuit. Its vector PDF labels FPC pins 4/5/20/21 as
VGL/VGH/VSH/PREVGH; the panel pin table recorded in the project selection
(`docs/component-selection.md` §C3, GDEY029T94 Rev 1.0 §5 p8) uses
NC/VSH2/VSH1/VGH. The reference board prints L1 `10uH` and selected
capacitors `1uF/50V`, whereas the panel application figure's recorded
requirements are `47uH/500mA` and `1uF/25V`. The reference corroborates
D1–D3's printed MBR0530 identity, the vector cathode directions
(left/right/right), and separate VSL and PREVGL labels at pins 22/23.
**The panel-side contrasts here are based on the existing project review,
not a newly fetched or independently traced panel PDF this run.** No pump
conductor edges were accepted from the abandoned experimental graph tracer.
Do not copy DESPI-C02 values or remaining pin assignments into the panel
schematic. The GDEY029T94 §12 endpoint-by-endpoint review is still outstanding.

## Readiness and verification tiers

- Static manufacturer-document probe: exit 0, 24 FPC row assertions plus
  diode/value/contrast assertions pass on the hash-pinned DESPI PDF.
- Existing native XML blocker gate (not a fresh export): exit 3,
  `MISSING_PART_IDENTITY` 17, `PLACEHOLDER_FOOTPRINT` 21,
  `DECLARED_UNVERIFIED` 41. Blocker fixture tests: 10 passed.
- Native KiCad ERC/netlist export: **not rerun this cycle**. The local
  `kicad-cli` wrapper requires unavailable Docker image
  `parts-tally-kicad:9-arm64` (Docker pull denied). The schematic itself was
  not edited; historical ERC reports are not current-run evidence.
- Simulation/host: Python document assertions only; no SPICE run.
  Bench measurements: none. Field testing: none. No PCB exists, so DRC is
  not applicable. No BOM identity, footprint, stock, pricing or order claims
  follow from this reference-document comparison.

**Verdict:** #3 remains blocked and PR #12 must stay draft/unmerged. Resolve
panel-specific topology, real part identities, footprints, and independent
schematic review before manufacturing or progressing dependent issues.
