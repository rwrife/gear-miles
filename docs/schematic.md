# Issue 3 schematic draft — blocked, do not build

**NOT FOR FABRICATION OR POWER-UP. Issue #3 is not complete.**
The editable KiCad project, schematic, local symbols, native ERC reports,
XML netlist and supplemental PDF are preserved for review under
`hardware/kicad/`. The PCB is deliberately absent: layout is issue #5 and
must not start from this rejected circuit. No BOM order/export is authorized.

## Blocking findings

| Finding | Confidence / evidence | Required resolution |
|---|---|---|
| D2/D3 candidate polarity does not produce the intended negative rail: D2 A=GND/K=PUMP, D3 A=PUMP/K=PREVGL | High engineering confidence; current exported pin map and rectifier polarity reasoning, **not simulation** | Independently trace the manufacturer GDEY029T94 §12 p29 diagram; correct both diode directions and audit physical pad numbers. The incorrect candidate remains visibly marked so its current zero-ERC result cannot be mistaken for approval. |
| Panel support network is not reliably transcribed, including J4 pin22 VSL versus pin23 VGL, reservoir and flying-capacitor endpoints/values | Unverified; the actual manufacturer p29 is a raster image. Automated/agent readings disagreed | Complete an independent edge-by-edge comparison against the original diagram; do not infer missing edges from this draft. |
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
**internal consistency only**, not independent circuit validation. The test
currently accepts the rejected pump map; that limitation is intentional and
explicit until the independent circuit review replaces it.

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
