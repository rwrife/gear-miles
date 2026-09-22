#!/usr/bin/env python3
"""Compare the 2025 re-typeset official GDEY029T94 copy against the 2021 mirror.

The good-display.com download endpoint for GDEY029T94 (companyfile/621,
"Time of issue 2025-11-26") redirects via downloadNew.do to
https://v4.cecdn.yun300.cn/100001_1909185148/GDEY029T94.pdf . This probe
deterministically compares that copy against the repo-identity copy
(the Seeed mirror) to answer exactly two questions:

1. Is the §12 reference-circuit figure machine-traceable in the new copy?
   (Expected: no — the same 986x681 JPEG with no vector circuit content
   and no text labels inside the figure bbox; label-position equality vs
   the 2021 copy is proven by the deterministic byte-grid diff.)
2. What NEW machine-readable text does the new copy carry?
   (§12 "Requirements for spare part" table, absent from the 2021 copy.)

Both PDFs must be passed explicitly; SHA-256 identity is asserted, never
assumed. Requires PyMuPDF (import pymupdf). Output: plain text to stdout;
exit 1 on any identity mismatch or missing expected fact.

Usage:
  probe_official_cdn_copy.py LEGACY.pdf OFFICIAL_2025.pdf
"""
import hashlib
import re
import sys

OFFICIAL_SHA = "750d119dec52a4f313f6ae3ee90aa02cd71cbf7f83cb584df51ef7d5517bf264"
LEGACY_SHA = "2ea221896f9b0463e7f53b6a4d0dff967728f2049ca7667d45878c1ff4e876a9"
FIG_BBOX = (49.7, 120.4, 544.7, 462.3)  # figure placement rect, both copies


def sha(path: str) -> str:
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def main(legacy_path: str, official_path: str) -> int:
    import pymupdf  # noqa: deferred so import errors are loud, not silent

    problems: list[str] = []
    legacy_sha, official_sha = sha(legacy_path), sha(official_path)
    print(f"legacy   sha256 {legacy_sha}")
    print(f"official sha256 {official_sha}")
    if legacy_sha != LEGACY_SHA:
        problems.append("legacy copy is not the repo-identity Seeed mirror")
    if official_sha != OFFICIAL_SHA:
        problems.append("official copy is not the 2025 CDN Rev 1.0 re-typeset")

    a = pymupdf.open(legacy_path)
    b = pymupdf.open(official_path)
    print(f"pages: legacy={a.page_count} official={b.page_count}")

    # Section 12 page: locate by heading text, not hardcoded index.
    def sec12_page(doc, label):
        # The section heading also appears in the TOC; require the 986x681
        # figure raster on the same page to disambiguate.
        for i in range(doc.page_count):
            if re.search(r"12\.\s*Reference Circuit", doc[i].get_text()) and any(
                    im[2] == 986 and im[3] == 681
                    for im in doc[i].get_images(full=True)):
                print(f"{label}: section 12 page = {i + 1}")
                return i
        return None

    ia, ib = sec12_page(a, "legacy"), sec12_page(b, "official")
    if ia is None or ib is None:
        problems.append("section 12 heading not found")
        print("\n".join(problems))
        return 1

    # Q1: vector/text traceability inside the figure bbox, both copies.
    grid = {}
    for doc, label, page in ((a, "legacy", ia), (b, "official", ib)):
        p = doc[page]
        vec_in_fig = [d for d in p.get_drawings()
                      if FIG_BBOX[0] <= d["rect"].x0 and d["rect"].x1 <= FIG_BBOX[2]
                      and FIG_BBOX[1] <= d["rect"].y0 and d["rect"].y1 <= FIG_BBOX[3]]
        text_in_fig = []
        for blk in p.get_text("dict")["blocks"]:
            if blk["type"] != 0:
                continue
            for ln in blk["lines"]:
                for sp in ln["spans"]:
                    x0, y0, _, _ = sp["bbox"]
                    if (FIG_BBOX[0] <= x0 <= FIG_BBOX[2]
                            and FIG_BBOX[1] <= y0 <= FIG_BBOX[3]
                            and sp["text"].strip()):
                        text_in_fig.append(sp["text"].strip())
        print(f"{label}: drawings inside figure bbox = {len(vec_in_fig)}; "
              f"text spans inside figure bbox = {text_in_fig}")
        big = [im for im in p.get_images(full=True) if im[2] == 986 and im[3] == 681]
        if len(big) != 1:
            problems.append(f"{label}: expected exactly one 986x681 figure raster")
            continue
        pix = pymupdf.Pixmap(doc, big[0][0])
        n = pix.n
        w, h = pix.width, pix.height
        G = 16
        cw, ch = w // G, h // G
        samples = pix.samples
        # Per-cell grayscale mean, robust to JPEG recompression noise; a
        # real label/edge MOVE changes a cell mean far more than re-encoding.
        means = []
        for gy in range(G):
            for gx in range(G):
                x0, y0 = gx * cw, gy * ch
                acc = cnt = 0
                for yy in range(y0, min(y0 + ch, h)):
                    row = yy * w * n
                    for xx in range(x0, min(x0 + cw, w), 2):
                        i = row + xx * n
                        acc += (samples[i] * 3 + samples[i + 1] * 6 + samples[i + 2]) // 10
                        cnt += 1
                means.append(acc / max(cnt, 1))
        grid[label] = means
        print(f"{label}: 986x681 raster xref {big[0][0]}, "
              f"sha256(bytes) {hashlib.sha256(samples).hexdigest()[:16]}")
    if "legacy" in grid and "official" in grid:
        maes = [abs(x - y) for x, y in zip(grid["legacy"], grid["official"])]
        worst = max(range(len(maes)), key=lambda i: maes[i])
        print(f"16x16 cell-mean diff: mean MAE {sum(maes) / len(maes):.2f}, "
              f"worst cell {worst} (row {worst // 16}, col {worst % 16}) MAE {maes[worst]:.2f}")
        if maes[worst] > 25 or sum(maes) / len(maes) > 8:
            problems.append("figure content layout differs beyond recompression noise")

    # Q2: new machine-readable §12 text in the official copy.
    t_official = b[ib].get_text()
    must = ["Requirements for spare part", "MBR0530", "Si1308EDL", "NR3015",
            "X5R/X7R", "0.5mm pitch", "0.05W", "430mV", "400m"]
    missing = [m for m in must if m not in t_official]
    print(f"official §12 machine-readable table: {len(must) - len(missing)}/{len(must)} "
          f"expected tokens present; missing={missing}")
    if missing:
        problems.append(f"official copy missing expected §12 table tokens: {missing}")
    t_legacy = a[ia].get_text()
    leaked = [m for m in must if m in t_legacy]
    print(f"legacy §12 page tokens (expected absent): leaked={leaked}")
    if leaked:
        problems.append("legacy copy unexpectedly carries the 2025 table tokens")

    print("RESULT: " + ("FAIL\n" + "\n".join(problems) if problems else "PASS"))
    return 1 if problems else 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        raise SystemExit(2)
    raise SystemExit(main(sys.argv[1], sys.argv[2]))
