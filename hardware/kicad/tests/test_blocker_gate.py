#!/usr/bin/env python3
"""Synthetic host fixtures exercise the gate; none are hardware evidence."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
GATE = ROOT / "hardware/kicad/tools/check_blockers.py"


def fixture(rejected=False):
    """Minimal invented test input, deliberately NOT a complete panel circuit."""
    root = ET.Element("export")
    components = ET.SubElement(root, "components")
    for ref in ("D2", "D3", "J4"):
        comp = ET.SubElement(components, "comp", ref=ref)
        ET.SubElement(comp, "footprint").text = "TEST_ONLY:fake-footprint"
        for name, value in (("Manufacturer", "TEST_ONLY"), ("MPN", "TEST_ONLY"),
                            ("Validation_Status", "REVIEWED_SCHEMATIC")):
            ET.SubElement(comp, "property", name=name, value=value)
    nets = ET.SubElement(root, "nets")
    nodes = [("D2", "1", "K", "PUMP" if rejected else "GND"),
             ("D2", "2", "A", "GND" if rejected else "PUMP"),
             ("D3", "1", "K", "PREVGL" if rejected else "PUMP"),
             ("D3", "2", "A", "PUMP" if rejected else "PREVGL"),
             ("J4", "22", "VSL", "PREVGL" if rejected else "VSL"),
             ("J4", "23", "VGL", "PREVGL")]
    for ref, pin, function, name in nodes:
        net = next((n for n in nets if n.get("name") == name), None)
        if net is None:
            net = ET.SubElement(nets, "net", name=name)
        ET.SubElement(net, "node", ref=ref, pin=pin, pinfunction=function)
    return root


class BlockerGateTests(unittest.TestCase):
    def run_gate(self, root):
        with tempfile.TemporaryDirectory() as tmp:
            net, report = Path(tmp) / "fixture.xml", Path(tmp) / "report.json"
            net.write_bytes(ET.tostring(root) if not isinstance(root, bytes) else root)
            result = subprocess.run(
                [sys.executable, str(GATE), str(net), "--output", str(report)],
                capture_output=True, text=True,
            )
            data = json.loads(report.read_text()) if report.exists() else {}
            return result.returncode, data, result.stdout + result.stderr

    def test_rejected_candidate_cannot_report_success(self):
        code, data, output = self.run_gate(fixture(rejected=True))
        self.assertEqual(code, 3, output)
        self.assertEqual({item["code"] for item in data["findings"]}, {
            "NEGATIVE_PUMP_CLAMP", "NEGATIVE_PUMP_RECTIFIER", "PANEL_RAIL_SHORT"})
        self.assertEqual(data["status"], "BLOCKED")

    def test_limited_pass_is_not_build_approval(self):
        code, data, output = self.run_gate(fixture())
        self.assertEqual(code, 0, output)
        self.assertEqual(data["status"], "LIMITED_CHECKS_PASSED")
        self.assertIn("not-design-approval", data["scope"])

    def test_declared_metadata_blockers_survive_topology_fix(self):
        root = fixture()
        comp = root.find("./components/comp")
        assert comp is not None
        footprint = comp.find("footprint")
        assert footprint is not None
        footprint.text = "Gear:TBD_LAYOUT_BLOCKED"
        for prop in comp.findall("property"):
            prop.set("value", "UNVERIFIED_DO_NOT_BUILD" if prop.get("name") == "Validation_Status" else "TBD")
        code, data, output = self.run_gate(root)
        self.assertEqual(code, 3, output)
        self.assertEqual({f["code"] for f in data["findings"]}, {
            "MISSING_PART_IDENTITY", "PLACEHOLDER_FOOTPRINT", "DECLARED_UNVERIFIED"})

    def test_invalid_input_never_reports_a_pass(self):
        for raw in (b"broken XML", b"<export/>", b"<not-a-netlist/>"):
            with self.subTest(raw=raw):
                code, data, output = self.run_gate(raw)
                self.assertEqual(code, 2, output)
                self.assertEqual(data["status"], "INVALID_INPUT")

    def test_missing_target_pin_is_blocked(self):
        root = fixture()
        for net in root.findall("./nets/net"):
            for node in list(net):
                if node.get("ref") == "D2" and node.get("pinfunction") == "A":
                    net.remove(node)
        code, data, output = self.run_gate(root)
        self.assertEqual(code, 3, output)
        self.assertIn("NEGATIVE_PUMP_CLAMP", {f["code"] for f in data["findings"]})

    def test_ambiguous_target_function_is_invalid(self):
        root = fixture()
        net = root.find("./nets/net")
        assert net is not None
        ET.SubElement(net, "node", ref="D2", pin="99", pinfunction="A")
        code, data, output = self.run_gate(root)
        self.assertEqual(code, 2, output)
        self.assertEqual(data["status"], "INVALID_INPUT")


    def test_exclusion_values_do_not_silently_bypass_metadata(self):
        for value in (None, "true", "false", "0", "", "garbage"):
            with self.subTest(value=value):
                root = fixture()
                comp = root.find("./components/comp")
                assert comp is not None
                mpn = comp.find("property[@name='MPN']")
                assert mpn is not None
                mpn.set("value", "TBD")
                prop = ET.SubElement(comp, "property", name="exclude_from_bom")
                if value is not None:
                    prop.set("value", value)
                code, data, output = self.run_gate(root)
                expected = 0 if value in (None, "true") else 3 if value in ("false", "0") else 2
                self.assertEqual(code, expected, output)

    def test_duplicate_property_is_invalid(self):
        root = fixture()
        comp = root.find("./components/comp")
        assert comp is not None
        ET.SubElement(comp, "property", name="MPN", value="TBD")
        code, data, output = self.run_gate(root)
        self.assertEqual(code, 2, output)
        self.assertEqual(data["status"], "INVALID_INPUT")

    def test_empty_footprint_is_blocked(self):
        root = fixture()
        footprint = root.find("./components/comp/footprint")
        assert footprint is not None
        footprint.text = None  # Serializes as <footprint />.
        code, data, output = self.run_gate(root)
        self.assertEqual(code, 3, output)
        self.assertIn("PLACEHOLDER_FOOTPRINT", {f["code"] for f in data["findings"]})

    def test_unknown_validation_state_remains_blocked(self):
        for value in ("TEST_ONLY", "REJECTED", "FAILED", "", "approved"):
            with self.subTest(value=value):
                root = fixture()
                prop = root.find("./components/comp/property[@name='Validation_Status']")
                assert prop is not None
                prop.set("value", value)
                code, data, output = self.run_gate(root)
                self.assertEqual(code, 3, output)
                self.assertIn("DECLARED_UNVERIFIED", {f["code"] for f in data["findings"]})


if __name__ == "__main__":
    unittest.main()
