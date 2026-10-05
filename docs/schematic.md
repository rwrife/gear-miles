# Issue 3 schematic draft — blocked, do not build

**NOT FOR FABRICATION OR POWER-UP. Issue #3 is not complete.**
The editable KiCad project, schematic, local symbols, native ERC reports,
XML netlist and supplemental PDF are preserved for review under
`hardware/kicad/`. The PCB is deliberately absent: layout is issue #5 and
must not start from this rejected circuit. No BOM order/export is authorized.

## Blocking findings

| Finding | Confidence / evidence | Required resolution |
|---|---|---|
| D2/D3 negative-pump polarity | **Corrected 2026-09-28:** D2 A=PUMP/K=GND, D3 A=PREVGL/K=PUMP. Standard negative charge-pump topology is independently corroborated by the official Good Display `DESPI-C02_SCH V1.0.pdf` vector schematic (D2/D3 cathode bars opposite D1); deterministic source/geometry evidence is in `hardware/kicad/reports/2026-09-28/`. This is static reference-design evidence, **not simulation or bench validation**. | Topology blocker cleared. Keep diode A/K and physical pad mapping under regression tests; do not claim analog validation until remaining panel-network endpoints/values are verified. |
| Panel support network is not reliably transcribed: reservoir and flying-capacitor endpoints/values still untraced | **VSL/VGL split applied 2026-09-20:** §5 p8 text (separate VSL/VGL capacitor pins) + figure-label row association (2026-09-19) + white-region topology corroboration (2026-09-20, `hardware/kicad/reports/2026-09-20/`): J4.22 now has its own `VSL` net and 1uF/25V bypass C14; J4.23 keeps `PREVGL`. Panel-generator ownership of the VSL rail is inferred from §5 text, not a traced p29 edge. Full edge list still outstanding; the p29 raster is not machine-traceable at native resolution. | Complete the remaining edge-by-edge comparison of the p29 figure (reservoir/flying-cap endpoints/values); validate the VSL cap placement against the figure before any build; do not infer missing edges from this draft. |
| Selected XUNPU FPC evidence not present locally | Deterministic document identity: local PDF names Kinghelm KH-FG0.5-H2.0-24PIN, SHA in manifest | Retrieve the actual selected connector drawing and verify pitch, contact side, numbering and land pattern. J4 remains TBD. |
| J2/J3, L1, R5/R7/R8 and C5-C13 lack exact manufacturer-backed selection | Deterministic missing metadata in symbol properties | Obtain manufacturer evidence and populate Manufacturer/MPN; retain PLANNING_ONLY until live sourcing. |
| Button and several connector/passive footprints are unresolved | Deterministic source inspection | `Gear:TBD_LAYOUT_BLOCKED` is an empty, conspicuous non-production placeholder, **not** a valid land pattern. Replace with manufacturer-derived footprints and pin-to-pad checks before issue #5. |
| TVS clamp does not establish GPIO safety | UMW Jan2025 PDF p1 lists 14 V clamping, above the ESP32 logic supply; source has 100-ohm series impedance | Calculate and qualify injection current/protection topology; do not claim E3/E5 are satisfied by device-level ESD ratings alone. |

## Corrections supported this run

- MICRONE ME6217 V02 p2: SOT-89-5 pins **1 CE, 2 VSS, 3 NC,
  4 VIN, 5 VOUT**. CE and VIN connect to USB_5V; MCU EN uses a separate
  10 kOhm/1 uF RC. The abandoned earlier draft used the wrong LDO pin map.
- Espressif WROOM-02 v1.7 Table 3-1 maps all 19 module pads. C3 Series v2.4
  Table 3-3 recommends GPIO2 pulled high and requires GPIO8 high when GPIO9
  is low for download mode. GPIO2/GPIO8/BTN_A now have resistor pull-ups;
  TP9 exposes EN_RC. These are document/static checks, not boot tests.
- Vishay document 63399 Rev C p1 identifies **SC-70/SOT-323**, not SOT-23,
  for Si1308EDL, pins 1 gate / 2 source / 3 drain. Q1 now carries the exact
  ordering code Si1308EDL-T1-GE3 and corresponding SC-70 footprint.
- The actual UMW TVS PDF is locally available and text-extractable. Earlier
  claims that only a Nexperia family PDF exists are superseded for this part.
- All-passive IC symbols and an A4 drawing with off-page components were
  rejected. Current IC pins have electrical roles; the A1 PDF bounds are
  tested. Power flags are excluded from BOM/PCB; test pads are zero-BOM.

See [datasheet manifest](datasheet-sources.md) for exact hashes and URLs.
The historical issue #2 selection record is retained, with an explicit
warning; its prices, current calculations and availability are not adopted
as new evidence. No live distributor query was completed this run.

## What the automated checks do — and do not prove

`test_netlist.py` reads the editable source via a fresh native KiCad XML
export, checks the candidate map, and rejects a deleted J1 A9 connection.
For noncritical parts it derives expectations from the generator. This is
**internal consistency only**, not independent circuit validation. D2/D3 are
now explicit critical expectations matching the 2026-09-28 vector-backed
correction; mutation coverage remains synthetic host evidence.

`test_schematic_bounds.py` checks source placement/text anchors;
`test_pdf_bounds.py` checks extracted word bounds. Neither is a complete
visual readability/overlap review. Native ERC checks modeled electrical
rules and symbol consistency; it cannot establish analog pump operation,
component ratings, footprint geometry or physical panel compatibility.

The CI workflow checks deterministic regeneration, native ERC with
`--exit-code-violations`, netlist consistency and PDF bounds. Green CI must
**not** remove draft status or permit merge while the blockers remain.
Actual local run output is in `hardware/kicad/reports/verification.txt`.

## 2026-09-15: blocker gate added, circuit still rejected

This run repaired the CI evidence gap, **not the circuit**. No electrical
connections, component selections or footprints were changed. The PR remains
a draft and issue #3 remains open; no new issue work started while its review
blockers remain. Do not order, lay out, fabricate or power this candidate.

`check_blockers.py` independently reads a fresh native XML export, without
importing the schematic generator or its candidate expectations. Its separate
`electrical-blockers` CI job runs even for draft PRs and fails (exit 3) on:

- D2 reversed negative-pump clamp orientation;
- D3 reversed negative-pump rectifier orientation;
- J4 VSL/VGL missing or directly shorted;
- missing/TBD Manufacturer or MPN (16 BOM components this run);
- empty/TBD footprints (20 BOM components);
- validation status other than `REVIEWED_SCHEMATIC` (40 BOM components this run).

These are **six grouped findings**, not six bad components or ERC violations.
Power flags and bare-copper test points use native BOM-exclusion metadata:
a valueless marker (as emitted by KiCad 9), or explicit `true`, excludes;
`false` and `0` do not. Other exclusion values and duplicate properties are
rejected. Malformed XML, missing required envelope/attributes, duplicate
references/properties, and ambiguous target functions exit 2; this is **not**
full XML-schema validation. Exit/report guarantees require a writable report
destination. The only pass status is
`LIMITED_CHECKS_PASSED`: it is **not** design approval. Property strings do not
authenticate an independent review, a datasheet citation or a physical package;
absence of these known blockers does not make the circuit safe. The check uses
D2/D3 functional A/K labels and project net names, not validated package pads.
Any topology/part rename requires explicit review of this targeted checker.

The two diode checks are **engineering topology reasoning, not simulation or
an independently traced manufacturer diagram**. A negative pump clamps the
flying node's positive excursion to ground and extracts charge from the negative
reservoir during its negative excursion. The present pair does the reverse.
The actual GDEY029T94 Rev 1.0 §5 p8 text was re-read: pin22 is negative source
drive VSL; pin23 VGL supplies negative gate drive, VCOM and VSL. Their direct
short remains a blocker pending the complete §12 p29 application trace. The
local PDF SHA-256 still matches the manifest; its raster circuit was **not**
successfully independently traced in this run. No component values were inferred
from an unreadable edge, and no unverified circuit correction was applied.

### Actual local execution (2026-09-15)

Raw log, native ERC reports and blocker JSON are committed in
`hardware/kicad/reports/2026-09-15/`. The log records the unchanged schematic
hash and pre-repair branch commit, not a claim that the new checker was already
in that baseline commit. The XML input hash is in the blocker report.

- KiCad 9.0.9 native ERC: **0 violations**, text and JSON; no new exclusions.
- Deterministic schematic/symbol regeneration: byte-identical.
- Candidate map + deleted-J1-A9 mutation and source/PDF bounds: pass.
- Blocker-detector host tests: **10 passed**, using explicitly synthetic fixtures.
  Reversed polarity, direct rail short, incomplete metadata, missing pins,
  malformed/empty input, ambiguous target functions, exclusion values,
  duplicate properties and unknown validation states are exercised.
- Actual candidate blocker check: **exit 3, BLOCKED, six groups**.
- KiCad MCP native netlist extraction: 53 symbols, 32 nets, 149 pin connections.

Reproduce from the repository root (last command must currently exit 3):

```sh
python3 hardware/kicad/tests/test_blocker_gate.py -v
kicad-cli sch export netlist --format kicadxml \
  -o /tmp/gear-miles.net hardware/kicad/gear-miles.kicad_sch
python3 hardware/kicad/tools/check_blockers.py /tmp/gear-miles.net \
  --output /tmp/electrical-blockers.json
```

**Remaining limits:** schematic-analyzer and BOM-manager scripts are absent in
this skill installation; native KiCad/MCP were used instead. No full design
review, structured extraction, SPICE, distributor sourcing, PCB/DRC, thermal,
EMC, firmware build, bench measurement or field testing was performed. No
SPICE executable was found. The native zero-ERC result remains strictly static.

## 2026-09-19: raster-evidence pass on the §12 p29 figure

Raw probe dumps, label geometry and interpretation are committed in
`hardware/kicad/reports/2026-09-19/` (`p29-raster-evidence-summary.md`).
Method: PyMuPDF 8x render of page 29 + RapidOCR label placement + raw
dark-run slice probes; no local tesseract, no vision-based reading was
claimed. SSD1680 (same-pinout controller family) mirrors were fetched only
as a consistency cross-reference — the panel DS still does not name its
driver IC, so that file is **not** identity or topology evidence for this
panel.

Result: the VSL/VGL split now has datasheet-text plus figure-label
corroboration (pin-22 has no shared `PREVGL` label row); the D2/D3 lead
endpoints could **not** be machine-traced (raster crossings are not
distinguishable from junctions at native resolution). **No schematic
connections, parts or footprints were changed by this pass.** The blocker
gate still exits 3 on the same six groups, which is the expected state.

Sharpened next step for issue #3: one bounded manual trace of the p29
pump edges (3.3 V rail → boost inductor/MOSFET switch node → positive-rail
rectifier + reservoir → negative pump diodes + flying cap → VSL and VGL
reservoirs; figure references D1-D3 are the driver figure's own numbers,
not this project's designators), then rename/split
`PREVGL` so J4 pin22 (VSL) gets its own net + capacitor, updating
`test_netlist.py` `CRITICAL_EXPECTED`, the `check_blockers.py` fixtures
and the on-sheet blocker text in the same change. The figure carries the
passive values (1uF/25V reservoirs, 4.7uF/25V at C4, 47uH/500mA L1,
2.2R and >1M resistors), so the same trace can retire part of the
MISSING_PART_IDENTITY list with figure-cited values before live sourcing.

### Addendum (2026-09-20): native 1:1 raster pass and VSL/VGL split

Raw native-1:1 dumps (embedded JPEG xref 187, no interpolation) and a
white-region connectivity trace are committed in
`hardware/kicad/reports/2026-09-20/` (`native-raster-and-vsl-split-summary.md`).
Findings: (1) pin-22 and pin-23 lead stubs sit in *different* enclosed white
regions, corroborating the label-row association that `PREVGL` names only the
pin-23 net; (2) the three figure diodes render pixel-identically
(bowtie-with-bar, same orientation) while serving different electrical roles,
so glyph orientation at this resolution cannot establish lead direction —
D2/D3 net endpoints remain datasheet-figure UNTRACED and their blockers
stand unchanged.

The bounded split step was applied to the editable generator: J4.22 (VSL)
now nets to a new own bypass cap C14 (1uF/25V figure-family value, identity
TBD) and a PWR5 ERC flag; J4.23 (VGL) keeps `PREVGL`. `test_netlist.py`
`CRITICAL_EXPECTED`, the `check_blockers.py` PANEL_RAIL_SHORT basis text, and
the on-sheet blocker text were updated together. Local evidence: ERC 0
violations (KiCad 9.0.9), netlist/bounds/blocker-fixture tests pass, PDF
bounds pass; the blocker gate now exits 3 on **five** groups (was six —
PANEL_RAIL_SHORT cleared, D2/D3 + metadata groups unchanged). This is a
datasheet-text-supported partial correction, **not** circuit validation or
build approval.

### Addendum (2026-09-22): official CDN re-typeset copy — spare-part table, no pump gain

The official good-display.com download for GDEY029T94 now serves a 2025
re-typeset copy of Rev 1.0 (SHA-256 `750d119d…7bf264`, revision history
still lists only 1.0/2021-03-15). Deterministic comparison
(`hardware/kicad/tools/probe_official_cdn_copy.py`, output in
`hardware/kicad/reports/2026-09-22/`):

- The §12 p29 figure is the same 986×681 raster content (OCR label sets
  identical at identical positions; 16×16 cell-mean MAE 0.53). **The pump
  trace outcome is unchanged: D2/D3 lead endpoints remain untraceable and
  the two NEGATIVE_PUMP_* blockers stand.**
- NEW machine-readable content: §12 "Requirements for spare part" table —
  D1—D3 = MBR0530, Q1 = Si1308EDL (both match drafted MPNs — corroboration
  of part *identity* only, never orientation/edges), L1 = "refer to NR3015,
  Io=500 mA(max)" (manufacturer-named inductor series; identity stays TBD
  until the specific NR3015A470 datasheet + live sourcing land), C1—C12
  0603/0805 X5R/X7R ≥25 V (explicitly permits the X5R dielectric the
  2026-09-21 shortlist flagged as a deviation — now manufacturer-sanctioned),
  P1 24-pin 0.5 mm.
- Scope caveat: the table's D1—D3 is the *figure's* designator range; this
  project's D1 is the USB ESD device and is not in that pump figure.
- Two real text deletions vs the 2021 copy (p9 optimal-storage-temp row,
  p37 24 h refresh note) — render-verified, not design-relevant.
- **No source, netlist, generator, or gate file was changed this pass**;
  the blocker gate remains exit 3 on the same five groups.

## 2026-09-28: DESPI-C02 vector evidence and D2/D3 correction

Good Display's official standalone `DESPI-C02_SCH V1.0.pdf` is a true vector
schematic, unlike the GDEY029T94 §12 embedded raster. The deterministic probe
`hardware/kicad/tools/probe_despi_vector_sch.py` fetched the CDN file
(SHA-256 `b1893766…26128b1`) and extracted diode glyph/conductor geometry:
D1's cathode bar is on the left, while D2/D3 cathode bars are on the right.
This independently corroborates the negative-pump topology expected by the
gate. Source and output are committed under
`hardware/kicad/reports/2026-09-28/`.

The editable source generator was corrected and regenerated in the same run:

- D2 is now `A=PUMP, K=GND` (clamps the positive PUMP excursion).
- D3 is now `A=PREVGL, K=PUMP` (extracts negative charge into PREVGL).
- `test_netlist.py` critical pin/net expectations changed with the source.
- `check_blockers.py` keeps both polarity checks as regression guards and
  records the vector-reference basis.
- Native ERC remains **0 violations**; host blocker tests remain **10/10**;
  netlist, source bounds and PDF bounds pass.
- The real blocker gate still exits **3**, now on **three** metadata/approval
  groups (was five): `MISSING_PART_IDENTITY` (17),
  `PLACEHOLDER_FOOTPRINT` (21), and `DECLARED_UNVERIFIED` (41).

This is static reference-design corroboration, not simulation or bench
validation. DESPI-C02 is a Good Display reference board, not proof that every
GDEY029T94 §12 reservoir/flying-cap endpoint and value has been transcribed.
The PR remains draft and the design remains not-for-build.

## 2026-10-05: reference-board pinout contrast (not panel validation)

A hash-pinned vector-text/glyph probe of the official DESPI-C02 schematic
(`hardware/kicad/reports/2026-10-05/`) confirms that this is a **same-family
reference board, not a pin-for-pin transcription target**: its pins 4/5/20/21
are VGL/VGH/VSH/PREVGH, versus the GDEY029T94 panel §5 p8 table's
NC/VSH2/VSH1/VGH. Its inductor and bypass ratings also differ from the panel
figure. D2/D3 orientation corroboration remains limited to topology; no
panel-specific conductor edge or unverified value was accepted from the
reference board. The available local `kicad-cli` wrapper could not rerun
ERC/netlist export this cycle because its Docker image was absent; the
schematic was unchanged, and the blocker gate on the already committed native
XML netlist still exited 3 on three metadata/approval groups. Neither historical
zero-ERC nor a reference-board assertion is fabrication approval.

## Verification limits

- **Static:** native ERC, netlist consistency, geometry bounds, targeted
  raw manufacturer-PDF checks and Markdown lint. Not a full design review.
- **Simulation/host:** Python host assertions only; no SPICE simulator was
  found (`which ngspice ltspice xyce` returned no executable).
- **Bench measurements:** none. No assembled prototype, flashing, cadence,
  refresh, thermal, transient, power-loss or network-capture evidence.
- **Field testing:** none.
- **Not performed:** full schematic analyzer suite, structured datasheet
  extraction cache, lifecycle/stock/pricing verification, thermal/EMC,
  schematic-to-PCB cross-analysis, DRC, Gerber inspection. PCB-dependent
  work is inapplicable because no board exists; other work remains blocked
  behind the rejected schematic rather than being represented as completed.

The next executor must review the draft PR first, resolve these findings,
rerun the checks, and only then consider completing #3 and unblocking #4/#5.
