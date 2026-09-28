#!/usr/bin/env python3
"""Probe Good Display DESPI-C02 vector schematic for pump-diode edge evidence.

Purpose
-------
Deterministically fetch and inspect the standalone DESPI-C02 schematic PDF
(vector source), then dump geometric evidence for the D1/D2/D3 area so issue-3
review can decide whether the data is exact-design evidence or same-family
reference-board corroboration.

This script is read-only. It does not edit KiCad files.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from urllib.request import Request, urlopen

import pymupdf

URL = "https://v4.cecdn.yun300.cn/100001_1909185148/DESPI-C02_SCH%20V1.0.pdf"
REFERER = "https://www.good-display.com/companyfile/DESPI-C02-SCH-30.html"
EXPECTED_SHA256 = "b1893766d212249429876aa1cb3e7cf929712ac2f2b262ce2da39948c26128b1"

# Pump-area bounds from the vector schematic page coordinate space.
ROI = pymupdf.Rect(250, 280, 900, 820)

# D1/D2/D3 anchor rows and nearby pin labels.
ROWS = [
    ("D1", 365.8),
    ("D2", 482.8),
    ("D3", 623.1),
]

TEXT_ROI = pymupdf.Rect(500, 300, 780, 760)


def fetch_pdf() -> bytes:
    req = Request(
        URL,
        headers={
            "User-Agent": "Mozilla/5.0",
            "Referer": REFERER,
        },
    )
    with urlopen(req, timeout=60) as resp:
        return resp.read()


def round_point(value: float) -> float:
    return round(value, 1)


def round_tuple(values):
    return tuple(round(float(v), 3) for v in values)


def collect_segments(page: pymupdf.Page):
    segments = []
    for d in page.get_drawings():
        color = d.get("color")
        if not color:
            continue
        # Circuit strokes are either pure blue (0,0,1) or navy (0,0,~0.5).
        # Keep both; reject other drawing colors such as black package graphics.
        if color[2] < 0.45 or color[2] <= max(color[0], color[1]):
            continue
        rect = d["rect"]
        # PyMuPDF marks zero-height/zero-width (purely axis-aligned) paths as
        # empty Rects, for which Rect.intersects() always returns False. Use an
        # explicit inclusive coordinate-range test instead.
        if not (
            rect.x1 >= ROI.x0
            and rect.x0 <= ROI.x1
            and rect.y1 >= ROI.y0
            and rect.y0 <= ROI.y1
        ):
            continue

        width = d.get("width")
        fill = d.get("fill")
        for item in d["items"]:
            if item[0] == "l":
                p1, p2 = item[1], item[2]
                seg = {
                    "kind": "line",
                    "x1": round_point(p1.x),
                    "y1": round_point(p1.y),
                    "x2": round_point(p2.x),
                    "y2": round_point(p2.y),
                    "width": round(width, 3) if width is not None else None,
                    "color": round_tuple(color),
                    "fill": round_tuple(fill) if fill else None,
                }
                if (
                    max(p1.x, p2.x) >= ROI.x0
                    and min(p1.x, p2.x) <= ROI.x1
                    and max(p1.y, p2.y) >= ROI.y0
                    and min(p1.y, p2.y) <= ROI.y1
                ):
                    segments.append(seg)
            elif item[0] == "re":
                r = item[1]
                if (r.x1 - r.x0) * (r.y1 - r.y0) > 100_000:
                    continue
                edges = [
                    (r.x0, r.y0, r.x1, r.y0),
                    (r.x1, r.y0, r.x1, r.y1),
                    (r.x1, r.y1, r.x0, r.y1),
                    (r.x0, r.y1, r.x0, r.y0),
                ]
                for x1, y1, x2, y2 in edges:
                    seg = {
                        "kind": "rect_edge",
                        "x1": round_point(x1),
                        "y1": round_point(y1),
                        "x2": round_point(x2),
                        "y2": round_point(y2),
                        "width": round(width, 3) if width is not None else None,
                        "color": round_tuple(color),
                        "fill": round_tuple(fill) if fill else None,
                    }
                    if (
                        max(x1, x2) >= ROI.x0
                        and min(x1, x2) <= ROI.x1
                        and max(y1, y2) >= ROI.y0
                        and min(y1, y2) <= ROI.y1
                    ):
                        segments.append(seg)
    return segments


def long_horizontals_for_row(segments, y_target: float):
    out = []
    for s in segments:
        if abs(s["y1"] - s["y2"]) > 0.2:
            continue
        if abs(s["y1"] - y_target) > 3.0:
            continue
        if abs(s["x2"] - s["x1"]) < 25:
            continue
        out.append(s)
    out.sort(key=lambda s: (min(s["x1"], s["x2"]), min(s["y1"], s["y2"])))
    return out


def nearby_segments(segments, x: float, y: float, radius: float = 3.0):
    out = []
    for s in segments:
        x1, y1, x2, y2 = s["x1"], s["y1"], s["x2"], s["y2"]
        if min(x1, x2) - radius <= x <= max(x1, x2) + radius and min(y1, y2) - radius <= y <= max(y1, y2) + radius:
            out.append(s)
    return out


def text_tokens(page: pymupdf.Page):
    tokens = []
    for word in page.get_text("words"):
        x0 = float(word[0])
        y0 = float(word[1])
        x1 = float(word[2])
        y1 = float(word[3])
        text = str(word[4])
        if TEXT_ROI.x0 <= x0 <= TEXT_ROI.x1 and TEXT_ROI.y0 <= y0 <= TEXT_ROI.y1:
            tokens.append(
                {
                    "text": text,
                    "x0": round_point(x0),
                    "y0": round_point(y0),
                    "x1": round_point(x1),
                    "y1": round_point(y1),
                }
            )
    return tokens


def main() -> int:
    pdf_bytes = fetch_pdf()
    sha = hashlib.sha256(pdf_bytes).hexdigest()

    report = {
        "source": {
            "url": URL,
            "referer": REFERER,
            "size_bytes": len(pdf_bytes),
            "sha256": sha,
            "expected_sha256": EXPECTED_SHA256,
            "sha_matches_expected": sha == EXPECTED_SHA256,
        }
    }

    doc = pymupdf.open(stream=pdf_bytes, filetype="pdf")
    page = doc[0]
    report["page"] = {
        "count": len(doc),
        "rect": {
            "x0": round_point(page.rect.x0),
            "y0": round_point(page.rect.y0),
            "x1": round_point(page.rect.x1),
            "y1": round_point(page.rect.y1),
        },
        "word_count": len(page.get_text("words")),
        "drawing_count": len(page.get_drawings()),
    }

    segments = collect_segments(page)
    report["pump_region"] = {
        "roi": {
            "x0": ROI.x0,
            "y0": ROI.y0,
            "x1": ROI.x1,
            "y1": ROI.y1,
        },
        "segment_count": len(segments),
        "rows": [],
        "anchors": {},
        "text_tokens": text_tokens(page),
    }

    for name, y in ROWS:
        report["pump_region"]["rows"].append(
            {
                "name": name,
                "y_target": y,
                "long_horizontals": long_horizontals_for_row(segments, y),
            }
        )

    anchors = {
        "D1_bar": (646.5, 366.0),
        "D1_apex": (671.0, 366.0),
        "D2_bar": (647.7, 483.0),
        "D2_apex": (671.0, 483.0),
        "D3_bar": (647.7, 623.0),
        "D3_apex": (671.0, 623.0),
        "N0PREVGL_label": (689.0, 518.0),
        "N0PREVGH_label": (690.0, 692.0),
        "P0D101": (642.0, 529.0),
        "P0D102": (563.0, 529.0),
        "P0D201": (563.0, 609.0),
        "P0D202": (642.0, 609.0),
        "P0D301": (563.0, 704.0),
        "P0D302": (642.0, 704.0),
    }
    for name, (x, y) in anchors.items():
        report["pump_region"]["anchors"][name] = nearby_segments(segments, x, y, radius=3.0)

    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
