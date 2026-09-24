#!/usr/bin/env python3
"""Probe L1 inductor sourcing avenues.

The GDEY029T94 §12 spare-part table (2025 CDN copy, verified 2026-09-22)
names L1 as "refer to NR3015, Io=500 mA(max)". Earlier passes searched for a
literal MPN "NR3015A470". This probe answers what that reference actually
is, deterministically and live:

1. The literal MPN NR3015A470 has no catalog entry on the jlcsearch
   community API (0 components).
2. NR3015 is Taiyo Yuden's series, proven on the manufacturer's own site:
   TYCOMPAS maps LSXBD3030QKT470M with "Previous Part Number : NR3015T470M".
3. The NR3015 47 uH member is rated 300 mA (320 mA temp-rise) per the
   LCSC/JLCPCB parts-library record for NR3015T470M (C87171), i.e. BELOW the
   panel table's Io=500 mA line -> a strict single-source read of the series
   reference fails the current requirement in the 3030 footprint.
4. Live catalog members that satisfy 47uH + 3x3mm carry at most 440 mA
   (FNR3015S470MT) — none meets 500 mA at that footprint.

Exit 0 = every expected fact observed; exit 1 = any mismatch (facts are
asserted, never assumed). Network failures also exit 1. No file in the
repository is read or written; results print to stdout.
"""
import json
import re
import sys
import urllib.parse
import urllib.request

UA = {"User-Agent": "Mozilla/5.0 (X11; Linux x86_64) gear-miles-sourcing-probe"}
TIMEOUT = 45


def get(url: str) -> bytes:
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=TIMEOUT) as resp:
        return resp.read()


def search(query: str) -> list[dict]:
    url = ("https://jlcsearch.tscircuit.com/api/search?q="
           + urllib.parse.quote(query) + "&full=true&limit=30")
    return json.loads(get(url)).get("components", [])


def main() -> int:
    problems: list[str] = []

    def expect(label: str, condition: bool, detail: str = "") -> None:
        print(f"{'PASS' if condition else 'FAIL'} {label}" + (f" | {detail}" if detail else ""))
        if not condition:
            problems.append(label)

    # 1. Literal MPN from the earlier shorthand is not a purchasable identity.
    for q in ("NR3015A470", "NR3015A470M"):
        comps = search(q)
        expect(f"api-no-entry {q}", comps == [], f"{len(comps)} results")

    # 2. Manufacturer mapping: Taiyo Yuden site carries the supersession line.
    ty = get("https://ds.yuden.co.jp/TYCOMPAS/ap/detail?pn=LSXBD3030QKT470M&u=M")
    ty_text = re.sub(r"\s+", " ", re.sub(r"<[^>]+>", " ", ty.decode("utf-8", "ignore")))
    expect("ty-supersession-line",
           "(Previous Part Number : NR3015T470M)" in ty_text,
           "TYCOMPAS maps LSXBD3030QKT470M -> NR3015T470M")

    # 3. The 47 uH NR3015 member is a 300 mA part in the shared parts library.
    jlc = get("https://jlcpcb.com/partdetail/TaiyoYuden-NR3015T470M/C87171")
    jlc_text = jlc.decode("utf-8", "ignore")
    expect("nr3015t470m-300ma",
           "NR3015T470M" in jlc_text and "300mA" in jlc_text and "47uH" in jlc_text,
           "LCSC/JLCPCB record: 47uH rated 300 mA < 500 mA line")

    # 4. No live 47uH 3x3mm member reaches 500 mA; best is 440 mA.
    series = search("NR3015")
    rows = []
    for c in series:
        desc = c.get("description") or ""
        if "47uH" in desc and ("3x3" in (c.get("package") or "")):
            amps = [int(m) for m in re.findall(r"(\d{3,4})mA", desc)]
            rows.append((c.get("mfr"), max(amps) if amps else None))
    best = max((a for _, a in rows if a), default=0)
    expect("no-47uh-3x3mm-meets-500ma", bool(rows) and best < 500,
           f"members={rows}")

    print(f"\n{'PROBE OK' if not problems else 'PROBE FAILED'} "
          f"({len(problems)} failed checks); evidence-only — no file changed")
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
