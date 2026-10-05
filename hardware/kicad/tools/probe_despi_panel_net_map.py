#!/usr/bin/env python3
"""Deterministic FPC rail map, spare-part legend and diode-direction probe of
Good Display's `DESPI-C02_SCH V1.0.pdf` (vector schematic).

Purpose
-------
Extract, from word/filled-glyph geometry of the hash-pinned vector PDF (no
raster OCR, no wire-tracing), the facts needed to judge whether DESPI-C02 is
an *exact copy* of the GDEY029T94 §12 application circuit or a *same-family
reference board*, and to lock the already-applied 2026-09-20/28 corrections:

  A. the 24-pin FPC pin -> rail-name table of DESPI-C02 itself (rail label =
     the word in the connector label column whose row sits one pitch below
     the pin-number word, observed at +11.5..+13.5 pt),
  B. the pump/boost spare-part legend (D1-D3 MBR0530, Q1 Si1308EDL,
     L1 10uH/1A, per-net bypass values),
  C. the three diode cathode-bar directions (filled vector glyph geometry).

Provenance outcome asserted below: DESPI-C02's connector assignment DIFFERS
from GDEY029T94 section 5 p8 at pins 4/5/20/21 and its boost/bypass values
differ (L1 10uH/1A vs 47uH/500mA; 1uF/50V vs 1uF/25V bypasses). It is a
same-family reference-board corroboration for charge-pump TOPOLOGY and part
IDENTITY only — it can never substitute for the panel figure's endpoint and
value map, and no GDEY029T94 §12 p29 edge was traced by this tool.

Every fact is asserted, never assumed: exit 0 only if all embedded
expectations held on the fetched, hash-verified PDF. Exit 1 on any mismatch
or network failure. Read-only; no repository file is touched.

Scope limits: static manufacturer-document analysis only — not simulation,
bench data, live sourcing, or proof that the Gear Miles candidate is safe to
build. Pin-level conductor connectivity in this PDF was deliberately NOT
traced: a 2026-09-30 graph-tracing attempt showed component-body strokes
(MOSFET stem, diode triangles) and pad-token offsets make naive tracing
unreliable; results there were discarded and are not used by this tool.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
from urllib.request import Request, urlopen

import pymupdf

URL = "https://v4.cecdn.yun300.cn/100001_1909185148/DESPI-C02_SCH%20V1.0.pdf"
REFERER = "https://www.good-display.com/companyfile/DESPI-C02-SCH-30.html"
EXPECTED_SHA256 = "b1893766d212249429876aa1cb3e7cf929712ac2f2b262ce2da39948c26128b1"

# DESPI-C02's OWN connector table (measured from the PDF word grid).
EXPECTED_RAIL_TABLE = {
    1: "NC", 2: "GDR", 3: "RESE", 4: "VGL", 5: "VGH", 6: "TSCL", 7: "TSDA",
    8: "BS", 9: "BUSY", 10: "RES", 11: "D/C", 12: "CS", 13: "SCLK", 14: "SDI",
    15: "VDDIO", 16: "VCI", 17: "VSS", 18: "VDD", 19: "VPP", 20: "VSH",
    21: "PREVGH", 22: "VSL", 23: "PREVGL", 24: "VCOM",
}

# GDEY029T94 Rev 1.0 section 5 p8 rows that DIFFER on DESPI-C02 (panel value
# per GDEY029T94; these are contrast facts, not assertions about this PDF).
PANEL_CONTRASTS = {4: "NC", 5: "VSH2", 20: "VSH1", 21: "VGH"}

EXPECTED_DIODE_DIRECTIONS = {"D1": "left", "D2": "right", "D3": "right"}
EXPECTED_LEGEND = {
    "D1": "MBR0530", "D2": "MBR0530", "D3": "MBR0530",
    "Q1": "Si1308EDL", "L1": "10uH",
    "C6": "1uF/50V", "C7": "1uF/50V", "C9": "1uF/50V",
    "C11": "4.7uF/25V",
}


def fetch_pdf() -> bytes:
    req = Request(URL, headers={"User-Agent": "Mozilla/5.0", "Referer": REFERER})
    with urlopen(req, timeout=60) as resp:
        return resp.read()


def rail_table(page: pymupdf.Page) -> dict[int, str]:
    pins, rails = [], []
    for w in page.get_text("words"):
        t = w[4]
        x0, y0, x1, y1 = (float(v) for v in w[:4])
        cy = (y0 + y1) / 2.0
        if 1215 <= x0 <= 1240 and t.isdigit() and len(t) <= 2:
            pins.append((int(t), cy))
        elif 1265 <= x0 <= 1345 and not t.startswith("N0"):
            rails.append((t, cy))
    table: dict[int, str] = {}
    for pin, cy in pins:
        cand = sorted((abs(ry - cy - 12.0), t) for t, ry in rails
                      if 6.0 < ry - cy < 17.0)
        if cand:
            table[pin] = cand[0][1]
    return table


def diode_directions(page: pymupdf.Page) -> dict[str, str]:
    """Pair each filled diode triangle with its cathode bar; bar left of the
    triangle -> points left, else right. Map to D1/D2/D3 by the text label
    directly above each glyph row."""
    triangles, bars = [], []
    for d in page.get_drawings():
        color = d.get("color")
        if not color or color[2] < 0.45 or not d.get("fill"):
            continue
        items, r = d["items"], d["rect"]
        if (len(items) == 3 and all(it[0] == "l" for it in items)
                and 15 < r.width < 35 and 30 < r.height < 60):
            triangles.append(r)
        elif (len(items) == 1 and items[0][0] == "re"
              and r.width < 5 and 30 < r.height < 60):
            bars.append(r)
    direction: dict[str, str] = {}
    named: dict[str, str] = {}
    for tri in triangles:
        bar = min(bars, key=lambda b: abs((b.y0 + b.y1) / 2 - (tri.y0 + tri.y1) / 2))
        if abs((bar.y0 + bar.y1) / 2 - (tri.y0 + tri.y1) / 2) >= 5:
            continue
        direction[tri] = "left" if bar.x1 <= tri.x0 + 1 else "right"
    for w in page.get_text("words"):
        if w[4] in {"D1", "D2", "D3"}:
            ymid = (float(w[1]) + float(w[3])) / 2
            tri = min(triangles, key=lambda t: t.y0 - ymid if t.y0 > ymid else 9999)
            if 0 < tri.y0 - ymid < 60 and tri in direction:
                named[w[4]] = direction[tri]
    return named


def legend_values(page: pymupdf.Page) -> dict[str, str]:
    """Associate a component reference with its local printed value."""
    refs = [w for w in page.get_text("words") if w[4] in EXPECTED_LEGEND]
    vals = [w for w in page.get_text("words")
            if w[4] in set(EXPECTED_LEGEND.values())]
    out: dict[str, str] = {}
    for w in refs:
        rx, ry = (float(w[0]) + float(w[2])) / 2, (float(w[1]) + float(w[3])) / 2
        same_row, below = [], []
        for v in vals:
            vx, vy = (float(v[0]) + float(v[2])) / 2, (float(v[1]) + float(v[3])) / 2
            dx, dy = vx - rx, vy - ry
            if 0 < dx < 180 and abs(dy) < 5:
                same_row.append((dx, abs(dy), v[4]))
            elif 0 < dy < 90 and abs(dx) < 120:
                below.append((abs(dx), dy, v[4]))
        candidates = same_row or below
        if candidates:
            out[w[4]] = min(candidates)[2]
    return out


def main() -> int:
    problems: list[str] = []

    def expect(label: str, ok: bool, detail: str = "") -> None:
        print(f"{'PASS' if ok else 'FAIL'} {label}" + (f" | {detail}" if detail else ""),
              file=sys.stderr)
        if not ok:
            problems.append(label)

    pdf = fetch_pdf()
    sha = hashlib.sha256(pdf).hexdigest()
    expect("sha256-match", sha == EXPECTED_SHA256, sha[:16] + "…")
    doc = pymupdf.open(stream=pdf, filetype="pdf")
    page = doc[0]

    rails = rail_table(page)
    for pin in range(1, 25):
        expect(f"rail pin{pin}", rails.get(pin) == EXPECTED_RAIL_TABLE[pin],
               f"got {rails.get(pin)!r} want {EXPECTED_RAIL_TABLE[pin]!r}")
    expect("vsl-vgl-split", rails.get(22) == "VSL" and rails.get(23) == "PREVGL",
           "reference board keeps pin22 VSL and pin23 PREVGL as distinct nets")
    for pin, panel_val in PANEL_CONTRASTS.items():
        expect(f"contrast pin{pin}", rails.get(pin) != panel_val,
               f"DESPI rail {rails.get(pin)!r} differs from GDEY029T94 p8 '{panel_val}'")

    diodes = diode_directions(page)
    for ref, want in EXPECTED_DIODE_DIRECTIONS.items():
        expect(f"diode {ref} cathode", diodes.get(ref) == want,
               f"got {diodes.get(ref)!r} want {want!r}")

    legend = legend_values(page)
    for ref, want in EXPECTED_LEGEND.items():
        expect(f"legend {ref}", legend.get(ref) == want,
               f"got {legend.get(ref)!r} want {want!r}")
    expect("boost-inductor-contrast", legend.get("L1") == "10uH",
           "DESPI-C02 L1=10uH/1A vs GDEY029T94 figure L1=47uH/500mA")
    expect("rail-bypass-50v-contrast",
           all(legend.get(c) == "1uF/50V" for c in ("C6", "C7", "C9")),
           "reference board bypasses are 1uF/50V; panel figure uses 1uF/25V")

    report = {
        "source": {"url": URL, "sha256": sha,
                   "sha_matches_expected": sha == EXPECTED_SHA256},
        "fpc_pin_rail_table_despi": {str(k): v for k, v in sorted(rails.items())},
        "diode_cathode_directions": diodes,
        "legend_values": legend,
        "provenance_judgement": {
            "relation": "same-family reference-board corroboration, NOT an exact copy",
            "basis": [
                "Charge-pump part identity (D1-D3 MBR0530, Q1 Si1308EDL) and the "
                "complementary D2/D3 cathode direction corroborate the applied "
                "2026-09-28 orientation fix (topology + identity evidence only).",
                "DESPI-C02 keeps VSL (pin 22) and PREVGL (pin 23) as distinct "
                "nets, corroborating the applied 2026-09-20 VSL/VGL split.",
                "Its connector assignment differs from GDEY029T94 section 5 p8 at "
                "pins 4/5/20/21 (DESPI: VGL/VGH/VSH/PREVGH; panel: NC/VSH2/VSH1/"
                "VGH), so it is NOT the panel's own §12 application circuit.",
                "Boost inductor (10uH/1A vs 47uH/500mA) and bypass ratings "
                "(1uF/50V vs 1uF/25V) differ from the panel figure; the panel "
                "datasheet remains the normative source for values and endpoints.",
            ],
        },
        "consequence_for_issue_3": [
            "The p29 endpoint+value map remains untraced; DESPI evidence may not "
            "close MISSING_PART_IDENTITY values by substitution.",
            "DESPI values must NOT be copied into gear-miles symbol properties.",
        ],
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    print(f"\n{'PROBE OK' if not problems else 'PROBE FAILED'} "
          f"({len(problems)} failed checks); evidence-only — no repository file changed",
          file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
