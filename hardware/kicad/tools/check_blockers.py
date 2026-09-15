#!/usr/bin/env python3
"""Limited static blocker gate on a freshly exported KiCad XML netlist.

Does NOT import the candidate generator. Exit 3: blockers; 2: invalid input;
0: only these limited checks passed (not electrical/build approval).
"""

import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def inspect(root):
    components = root.findall("./components/comp")
    if root.tag != "export" or not components or not root.findall("./nets/net"):
        raise ValueError("expected nonempty KiCad XML components and nets")
    refs = [comp.attrib["ref"] for comp in components]
    if len(refs) != len(set(refs)):
        raise ValueError("duplicate component reference")
    targets = {("D2", "A"), ("D2", "K"), ("D3", "A"), ("D3", "K"),
               ("J4", "VSL"), ("J4", "VGL")}
    pins = {}
    for net in root.findall("./nets/net"):
        for node in net.findall("node"):
            key = (node.attrib["ref"], node.get("pinfunction", ""))
            if key[0] not in refs:
                raise ValueError("net refers to an absent component")
            if key not in targets:
                continue
            if key in pins:
                raise ValueError(f"ambiguous target function: {key}")
            pins[key] = net.attrib["name"]
    findings = []

    def require(code, condition, summary, evidence, confidence):
        if not condition:
            findings.append({"code": code, "summary": summary,
                             "evidence": evidence, "confidence": confidence})

    require("NEGATIVE_PUMP_CLAMP",
            pins.get(("D2", "A")) == "PUMP" and pins.get(("D2", "K")) == "GND",
            "D2 must clamp the positive PUMP excursion, not the negative excursion.",
            {"D2.A": pins.get(("D2", "A")), "D2.K": pins.get(("D2", "K")),
             "basis": "Forward conduction A to K; docs/schematic.md blocking findings. Engineering topology check, not a manufacturer-diagram trace."},
            "engineering-topology")
    require("NEGATIVE_PUMP_RECTIFIER",
            pins.get(("D3", "A")) == "PREVGL" and pins.get(("D3", "K")) == "PUMP",
            "D3 must extract charge from PREVGL during the negative PUMP excursion.",
            {"D3.A": pins.get(("D3", "A")), "D3.K": pins.get(("D3", "K")),
             "basis": "Forward conduction A to K; docs/schematic.md blocking findings. Does not validate component values or physical pad numbering."},
            "engineering-topology")
    vsl, vgl = pins.get(("J4", "VSL")), pins.get(("J4", "VGL"))
    require("PANEL_RAIL_SHORT", bool(vsl and vgl and vsl != vgl),
            "J4 VSL and VGL must be present and not directly shorted; full panel network review remains required.",
            {"J4.VSL": vsl, "J4.VGL": vgl,
             "basis": "GDEY029T94 Rev 1.0 section 5 p8: VSL negative source drive; VGL supply for negative gate drive, VCOM and VSL. See docs/datasheet-sources.md."},
            "datasheet-informed-topology")
    groups = {"MISSING_PART_IDENTITY": [], "PLACEHOLDER_FOOTPRINT": [],
              "DECLARED_UNVERIFIED": []}
    for comp in components:
        props = {}
        for prop in comp.findall("property"):
            name = prop.attrib["name"]
            if name in props:
                raise ValueError(f"duplicate property {comp.attrib['ref']}.{name}")
            props[name] = prop.get("value")
        if "exclude_from_bom" in props:
            exclusion = props["exclude_from_bom"]
            # Native KiCad 9 emits a valueless marker. Explicit false must NOT
            # skip checks; an explicit empty value is not the native marker.
            if exclusion not in (None, "true", "false", "0"):
                raise ValueError(f"invalid BOM exclusion for {comp.attrib['ref']}")
            if exclusion in (None, "true"):
                continue
        ref = comp.attrib["ref"]
        identities = [(props.get(key) or "").strip().upper() for key in ("Manufacturer", "MPN")]
        if any(not value or value.startswith("TBD") or value == "N/A" for value in identities):
            groups["MISSING_PART_IDENTITY"].append(ref)
        footprint = (comp.findtext("footprint") or "").strip()
        if not footprint or "TBD" in footprint.upper():
            groups["PLACEHOLDER_FOOTPRINT"].append(ref)
        if props.get("Validation_Status") != "REVIEWED_SCHEMATIC":
            groups["DECLARED_UNVERIFIED"].append(ref)
    for code, affected in groups.items():
        require(code, not affected,
                f"{len(affected)} BOM components retain {code.lower()} blockers.",
                {"references": sorted(affected), "basis": "Native netlist symbol properties; presence checks do not authenticate datasheet or review evidence."},
                "deterministic-metadata")
    return findings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("netlist", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    report: dict = {"scope": "limited-static-blocker-checks-not-design-approval"}
    try:
        raw = args.netlist.read_bytes()
        report["netlist_sha256"] = hashlib.sha256(raw).hexdigest()
        findings = inspect(ET.fromstring(raw))
        status, code = ("BLOCKED", 3) if findings else ("LIMITED_CHECKS_PASSED", 0)
    except (OSError, ET.ParseError, ValueError, KeyError) as error:
        status, code = "INVALID_INPUT", 2
        findings = [{"code": "INVALID_INPUT", "summary": str(error)}]
    report.update(status=status, findings=findings)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"{status}: {len(findings)} blocker groups; not electrical approval")
    for finding in findings:
        print(f"{finding['code']}: {finding['summary']}")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
