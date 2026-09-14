#!/usr/bin/env python3
"""Reject a generated PDF that has text outside its page bounding box."""

from __future__ import annotations

import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

pdf = Path(sys.argv[1])
xml = subprocess.run(["pdftotext", "-bbox", str(pdf), "-"], check=True, text=True, capture_output=True).stdout
root = ET.fromstring(xml)
pages = root.findall(".//{*}page")
if len(pages) != 1:
    raise SystemExit(f"expected one A1 schematic page, got {len(pages)}")
for page in pages:
    width, height = float(page.attrib["width"]), float(page.attrib["height"])
    for word in page.findall(".//{*}word"):
        x0, y0, x1, y1 = (float(word.attrib[key]) for key in ("xMin", "yMin", "xMax", "yMax"))
        if x0 < 0 or y0 < 0 or x1 > width or y1 > height:
            raise SystemExit(f"clipped PDF word {word.text!r}: {x0},{y0},{x1},{y1} outside {width},{height}")
print("one-page PDF text bounds are clean")
