# GDEY029T94 official CDN copy (2025 re-typeset) vs repo-identity copy — 2026-09-22

No schematic, netlist, generator, footprint, or blocker-gate logic changed
this pass. This folder is evidence only; the electrical-blockers gate stays
red by design (same 5 groups as 2026-09-20/21).

## 1. Discovery and provenance

- `https://www.good-display.com/companyfile/621.html` (product-page download
  entry for GDEY029T94) was fetched with a browser UA (the bare fetch returns
  a 587-byte interstitial). Its download button points at
  `/comp/xcompanyFile/downloadNew.do?appId=24&fid=792&id=621`, which responds
  with a `window.open` redirect to
  `https://v4.cecdn.yun300.cn/100001_1909185148/GDEY029T94.pdf`.
  Page metadata: "Time of issue 2025-11-26 16:25:58", 2.9 MB.
- Fetched 2026-09-22: 3,072,304 bytes, PDF 1.7, 37 pages,
  **SHA-256 `750d119dec52a4f313f6ae3ee90aa02cd71cbf7f83cb584df51ef7d5517bf264`**.
  (Repo copy `datasheets/GDEY029T94.pdf` / Seeed mirror:
  `2ea22189…e876a9`, Rev 1.0 2021-03-15.)
- The new copy's revision history table (p3) lists **only** Rev 1.0 /
  MAR.15.2021 — it is the same Rev 1.0 specification, re-typeset (new
  GOODISPLAY logo, table reflow, image re-export), not a spec revision.

## 2. Deterministic comparison (`probe_official_cdn_copy.py`, PyMuPDF 1.28)

Raw output: `official-cdn-copy-probe.txt` (exit 0, RESULT: PASS).

- §12 "Reference Circuit" is page 29 in both copies; figure placement rect
  identical (49.7, 120.4)–(544.7, 462.3).
- Inside the figure bbox the new copy has **0 vector drawings and no circuit
  labels** — the single text span is the `GOODISPLAY` watermark. The figure
  is again the 986×681 JPEG (xref 543). Its decoded-pixel bytes differ from
  the 2021 raster (mean abs diff ≈ 1.94; 3.3% of bytes differ by >16 — JPEG
  recompression), but a 16×16 cell-mean layout comparison gives mean MAE
  0.53 / worst cell 2.66, and deterministic OCR of both rasters returns the
  **same label set at identical positions** (PREVGL@461,32; D2@358,121;
  D3@359,254; PREVGH@460,274; VSH1/VSH2/VGH/VSL/VGL/VCOM/PREVGH21/PREVGL23
  all equal within ±4 px; same 1uF/25V, 4.7uF/25V, 47uH/500mA, 2.2R, >1M,
  MBR0530×3, Si1308EDL annotations).
- **Consequence: the 2025 copy does NOT re-open the pump trace.** D2/D3 lead
  endpoints are exactly as untraceable here as in the 2021 file
  (2026-09-21 conclusion stands: structurally untraceable from the official
  PDF by any parser).

## 3. New machine-readable content (the actual gain)

The 2021 page 29 carries no part-table text at all (only the section
heading; the figure's part list existed solely inside the raster). The 2025
page 29 adds a machine-readable **"Requirements for spare part"** table
(below the figure, y≈505–660 pt, verified via text-span dict extraction):

| Refs | Requirement (verbatim, span-verified) |
|------|----------------------------------------|
| C1—C12 | 0603/0805; **X5R/X7R**; Voltage Rating: ≥25 V |
| R1、R2 | 0603/0805; 1% variation; ≥0.05 W |
| D1—D3 | **MBR0530**: 1) Reverse DC Voltage ≥30 V 2) Io ≥500 mA 3) Forward voltage ≤430 mV |
| Q1 | **Si1308EDL**: 1) Drain-Source breakdown ≥30 V 2) Vgs(th) ≤1.5 V 3) Rds(on) ≤400 mΩ |
| L1 | **refer to NR3015**: Io = 500 mA (max) |
| P1 | 24 pins, 0.5 mm pitch |

How this touches the five gate groups (evidence only — nothing edited):

- **L1 (MISSING_PART_IDENTITY)**: the manufacturer itself names an
  inductor-series reference, "refer to NR3015" (Murata standard series code;
  NR3015A470… family = 1210 47 µH). This converts L1 from "figure gives a
  rating only" (2026-09-21 shortlist row) to "manufacturer-named series".
  Identity stays **TBD** per policy — a live datasheet pull of the specific
  NR3015A470 variant and live sourcing are required before symbol
  properties change. A 2026-09-05 jlcparts catalog search for NR3015
  returned **no exact match** (`fetched_live=false`, snapshot; nearest
  1210 47 µH/500 mA was an extended-tier Sunlord AWL3225FP470MTF) —
  candidate evidence only, not sourcing.
- **MISSING_PART_IDENTITY capacitor caveat SHARPENS**: the table permits
  **X5R** for C1—C12. The 2026-09-21 shortlist flagged Samsung
  CL05A105KA5NQNC's X5R dielectric as a deviation "vs the X7R draft value";
  per the manufacturer's own 2025 table, X5R is conformant for the panel
  pump caps, so that decision is now manufacturer-sanctioned (the draft's
  X7R marking is a stricter-than-required choice, not a datasheet
  requirement). Also 0603/0805 packages — the figure's 0402 assumption for
  C5—C11/C14 came from raster OCR context, and the table's 0603/0805 should
  be preferred when #5 chooses footprints.
- **D1—D3**: the table names **MBR0530** (already the drafted MPN) as the
  manufacturer-specified pump/charge diode for the reference circuit.
  Important scope caveat: the table covers **D1—D3**, and the project's D1
  is the USB ESD device (PESD5V0S1BA), not a diode in the manufacturer's
  pump figure — so the table's naming corroborates **D2/D3** part choice
  only; it does NOT validate D2/D3 orientation (endpoints remain
  untraceable, 2026-09-21 stands).
- **Q1**: confirms Si1308EDL (matches drafted MPN); same corroboration-only
  scope.
- **NEGATIVE_PUMP_CLAMP / NEGATIVE_PUMP_RECTIFIER groups unchanged**: the
  gate's two topology findings stay open — a spare-part table cannot trace
  edges. Gate continues to exit 3 on the same 5 groups.

## 4. Content deltas outside §12 (full 37-page token diff)

All other page text is re-typesetting noise (logo token, Dpi 125→112, line
reflow), except two **real deletions** in the 2025 copy, render-OCR-verified
(page-9 storage table row "Optimal Storage Temp TSTGo 23±2 °C" absent;
page-37 "refresh every 24 h to avoid ghosting" note absent). Neither is
referenced by this design's docs; both noted in
`docs/datasheet-sources.md`. No spec values relevant to our circuit changed.

## 5. Remaining paths to close the pump groups (unchanged ranking)

1. GoodDisplay-direct legible §12 figure (email request; the company
   download remains raster-only after this hunt) — or an owner decision to
   accept engineering-topology pump orientation with on-sheet basis
   (option (b) of 2026-09-21).
2. Live sourcing for the MISSING_PART_IDENTITY set (incl. L1 via
   NR3015A470 datasheet), then footprints via #5.

## 6. Verification actually run this pass

- `probe_official_cdn_copy.py`: exit 0 (output in this folder).
- OCR label-position diff of both 986×681 rasters (RapidOCR, deterministic
  host tool, same as 2026-09-19 passes).
- Text-span and token-set diffs across all 37 pages; page-9/p37 deletions
  render-verified at 150 dpi.
- ERC / netlist / blocker-gate / test_netlist **not** re-run: no source,
  generator, or netlist-relevant file was touched (precedent: 18c7e62).
- Not hardware evidence of any kind; no prices/stock cited (catalog rows
  explicitly marked snapshot 2026-09-05, `fetched_live=false`).
