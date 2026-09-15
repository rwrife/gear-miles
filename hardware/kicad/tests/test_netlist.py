#!/usr/bin/env python3
"""Reject incomplete or altered physical pin maps in the editable source."""

from __future__ import annotations

import copy
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SCH = ROOT / "hardware/kicad/gear-miles.kicad_sch"
NET = ROOT / "hardware/kicad/reports/gear-miles.net"
sys.path.insert(0, str(ROOT / "hardware/kicad/tools"))
from generate_schematic import PARTS  # noqa: E402

CRITICAL_EXPECTED = {
    "J1": {"A1": "GND", "A4": "USB_5V", "A5": "CC1", "A6": "USB_D+", "A7": "USB_D-", "A8": "NC", "A9": "USB_5V", "A12": "GND", "B1": "GND", "B4": "USB_5V", "B5": "CC2", "B6": "USB_D+", "B7": "USB_D-", "B8": "NC", "B9": "USB_5V", "B12": "GND", "S1": "GND"},
    "U1": {"1": "USB_D+", "2": "GND", "3": "USB_D-", "4": "USB_D-", "5": "USB_5V", "6": "USB_D+"},
    "U2": {"1": "USB_5V", "2": "GND", "3": "NC", "4": "USB_5V", "5": "+3V3"},
    "U3": {"1": "+3V3", "2": "EN_RC", "3": "INK_RST", "4": "INK_BUSY", "5": "INK_CLK", "6": "INK_MOSI", "7": "BTN_B", "8": "BOOT", "9": "GND", "10": "INK_CS", "11": "U0_RXD", "12": "U0_TXD", "13": "USB_D-", "14": "USB_D+", "15": "INK_DC", "16": "GPIO2_STRAP", "17": "CADENCE", "18": "BTN_A", "19": "GND"},
    "D1": {"1": "CADENCE_EXT", "2": "GND"},
    "J4": {"1": "NC", "2": "GDR", "3": "RESE", "4": "NC", "5": "VSH2", "6": "NC", "7": "NC", "8": "GND", "9": "INK_BUSY", "10": "INK_RST", "11": "INK_DC", "12": "INK_CS", "13": "INK_CLK", "14": "INK_MOSI", "15": "+3V3", "16": "+3V3", "17": "GND", "18": "EPD_VDD", "19": "NC", "20": "VSH1", "21": "PREVGH", "22": "PREVGL", "23": "PREVGL", "24": "VCOM"},
    "L1": {"1": "+3V3", "2": "EPD_SW"}, "Q1": {"1": "GDR", "2": "RESE", "3": "EPD_SW"},
    "D2": {"1": "PUMP", "2": "GND"}, "D3": {"1": "PREVGL", "2": "PUMP"}, "D4": {"1": "PREVGH", "2": "EPD_SW"},
    "C13": {"1": "EPD_SW", "2": "PUMP"}, "C8": {"1": "VSH2", "2": "GND"},
}
EXPECTED = {
    part[0]: {number: net for number, _, net in part[-1]}
    for part in PARTS
}
EXPECTED.update(CRITICAL_EXPECTED)


def export() -> None:
    NET.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["kicad-cli", "sch", "export", "netlist", "--format", "kicadxml", "-o", str(NET), str(SCH)], check=True, cwd=ROOT)


def actual_map() -> dict[str, dict[str, str]]:
    root = ET.parse(NET).getroot()
    result: dict[str, dict[str, str]] = {}
    for net in root.findall("./nets/net"):
        for node in net.findall("node"):
            result.setdefault(node.attrib["ref"], {})[node.attrib["pin"]] = net.attrib["name"]
    return result


def validate(actual: dict[str, dict[str, str]]) -> list[str]:
    failures: list[str] = []
    expected_refs = set(EXPECTED)
    unexpected_refs = set(actual) - expected_refs
    missing_refs = expected_refs - set(actual)
    if unexpected_refs:
        failures.append(f"unexpected physical references: {sorted(unexpected_refs)}")
    if missing_refs:
        failures.append(f"missing physical references: {sorted(missing_refs)}")
    for ref, expected in EXPECTED.items():
        got = actual.get(ref, {})
        if set(got) != {pin for pin, net in expected.items() if net != "NC"}:
            failures.append(f"{ref}: physical connected pins {sorted(got)} != expected {sorted(pin for pin, net in expected.items() if net != 'NC')}")
        for pin, net in expected.items():
            if net != "NC" and got.get(pin) != net:
                failures.append(f"{ref}.{pin}: expected {net}, got {got.get(pin)}")
    return failures


def main() -> None:
    export()
    actual = actual_map()
    failures = validate(actual)
    corrupted = copy.deepcopy(actual)
    corrupted["J1"].pop("A9", None)
    if not validate(corrupted):
        failures.append("negative mutation unexpectedly passed")
    if failures:
        print("\n".join(failures), file=sys.stderr)
        raise SystemExit(1)
    print("candidate-map consistency passed (NOT circuit validation); deleted-J1-A9 mutation rejected")


if __name__ == "__main__":
    main()
