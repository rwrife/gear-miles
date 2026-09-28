# L1 live-sourcing pass — 2026-09-24 (evidence only)

No schematic, netlist, footprint, gate, or test file changed this pass. The
blocker gate remains exit 3 on the same five groups. Raw strings/hashes:
`l1-sourcing-raw-extracts.txt`; deterministic probe (exit 0, 5/5 live checks):
`probe_l1_sourcing-output.txt` via `hardware/kicad/tools/probe_l1_sourcing.py`.
All distributor evidence retrieved **live 2026-09-24** unless marked otherwise
(supersedes the 2026-09-05 snapshot status of the shortlist rows it touches).

## What this pass closes

The 2026-09-22 note left L1 pending "the specific NR3015A470 datasheet +
live sourcing". That pending item was built on a misreading and is now
corrected, not fulfilled:

1. **"NR3015A470" is not a purchasable MPN.** Live catalog queries return 0
   components for it (and for `NR3015A470M`). Every web hit for the string
   resolves to `NR3015T470M` or to unrelated second-source series. The panel
   table's line is "refer to NR3015" + figure value 47 µH — a truncated
   *series* reference, so the hunt for a literal `NR3015A470` datasheet is
   closed as malformed, not as failed.
2. **Manufacturer correction.** `NR3015` is **Taiyo Yuden**'s series, proven
   on the manufacturer's own site: TYCOMPAS carries
   `LSXBD3030QKT470M (Previous Part Number : NR3015T470M)`. The 2026-09-22
   summary called it "Murata standard series code" — wrong; recorded here so
   future passes don't chase Murata documentation.
3. **The named member fails the table's own current line.** The NR3015 47 µH
   member `NR3015T470M` is rated **300 mA / 320 mA** (LCSC/JLCPCB parts-library
   record C87171, corroborated by distributor snippets). The panel table's
   `Io = 500 mA (max)` line is therefore **not satisfiable in the 3030
   footprint by the named series member** — verified live: no catalog
   47 µH/3x3mm member reaches 500 mA (best live members: FNR3015S470MT 440 mA
   C167766, SMNR3015-470MT 350 mA C135243 — both extended-tier, second-source,
   Shunxiangnuo family).

## What L1's symbol properties now say (and why they don't change)

L1 stays `Manufacturer=TBD`, `MPN=TBD`. This pass converted L1 from
"manufacturer-named series, identity pending a datasheet pull" to
**"named series conflicts with the same table's Io=500 mA line"** — an
electrical selection question, not a paperwork gap. A strict series-purchase
(NR3015T470M) would be a datasheet-violating part; a 500 mA-conformant part
(NRS4018/NRS5030 class, live-catalog evidence in raw extracts [9]) is a
footprint change. That is exactly the kind of decision that must NOT be
encoded silently into symbol properties:

- the pump figure's raster endpoints are still untraced (2026-09-21 stands),
  so the actual pump current L1 sees is unknown — Io≥500 mA may be the
  manufacturer's margin, not the circuit's real demand;
- footprint is a #5 decision;
- owner decision needed: (a) accept `NR3015T470M` 300 mA against the panel's
  own spare-part table, (b) accept a larger 500 mA-conformant footprint, or
  (c) obtain GoodDisplay clarification of the Io line's meaning.

## Side re-verifications (live, same API)

- Samsung `CL05A105KA5NQNC` C52923: still **Basic**, stock 4,519,174 — the
  2026-09-21 shortlist's capacitor row is live-confirmed (X5R sanction stands
  from the 2025 table; 0603/0805-vs-0402 footprint note stands).
- `NR3015T470M` C87171: the LCSC *website* product page still resolves, but
  the catalog API no longer returns the entry and the LCSC datasheet URL
  serves an HTML interstitial → LCSC-side stock/price for C87171 is **not
  claimed** (UNVERIFIED); do not treat the resolving page as availability.

## Gate-group impact (unchanged counts)

- `MISSING_PART_IDENTITY`: still 17 refs — no symbol property changed. The
  L1 sub-question is re-framed above; J2/J3, R5/R7/R8 and the caps still need
  owner/sourcing decisions under the same policy.
- Other four groups untouched by this pass.

## Honest limits

- Distributor evidence tier: community parts-library API (jlcsearch) plus one
  manufacturer site (TYCOMPAS) and one parts-library web record (C87171);
  no DigiKey/Mouser API keys were used this pass. Prices shown are current
  API unit-price snapshots, not quantity tiers.
- The 300 mA rating's *primary* Taiyo Yuden PDF could not be re-fetched
  cleanly this pass (Mouser-hosted catalog PDF returned a JS interstitial);
  the rating rests on the shared parts-library record plus corroborating
  distributor snippets — recorded as strong secondary, not primary PDF.
- This is catalog/datasheet evidence, not bench measurement. No pump
  orientation, current, or waveform claim is made or implied.
