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
