#!/usr/bin/env python3
"""Check the generated A1 landscape drawing stays inside the printable sheet."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SCH = (ROOT / "hardware/kicad/gear-miles.kicad_sch").read_text()

if '(paper "A1")' not in SCH:
    raise SystemExit("schematic must use A1 landscape")
for x, y in re.findall(r'\(symbol \(lib_id "[^"]+"\) \(at ([\d.]+) ([\d.]+) 0\)', SCH):
    if not (15 <= float(x) <= 826 and 15 <= float(y) <= 579):
        raise SystemExit(f"symbol outside A1 landscape bounds: {x}, {y}")
for x, y in re.findall(r'\(text "[^"]*" .*? \(at ([\d.]+) ([\d.]+) 0\)', SCH):
    if not (15 <= float(x) <= 826 and 15 <= float(y) <= 579):
        raise SystemExit(f"text outside A1 landscape bounds: {x}, {y}")
print("source placement and text anchors are within A1 landscape bounds")
