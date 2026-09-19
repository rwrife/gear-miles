#!/usr/bin/env python3
"""Generate the editable KiCad 9 schematic from the reviewed pin/net map."""

from __future__ import annotations

from pathlib import Path
import uuid

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "hardware" / "kicad" / "gear-miles.kicad_sch"
LIB = ROOT / "hardware" / "kicad" / "gear.kicad_sym"


def uid(key: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_URL, f"rwrife/gear-miles/issue-3/{key}"))


def q(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def effects(size: float = 1.27) -> str:
    return f'(effects (font (size {size} {size})))'


def pins(*items: tuple[str, str, str]) -> list[tuple[str, str, str]]:
    return list(items)


# ref, value, manufacturer, MPN, Datasheet URL/path, citation, footprint, pins
# A net named NC generates a KiCad no-connect marker, rather than a disconnected
# passive pin.  The FPC part is deliberately TBD: its alleged XUNPU evidence was
# a Kinghelm drawing, which cannot establish the selected connector footprint.
PARTS = [
    ("J1", "TYPE-C 16PIN 2MD(073)", "SHOU HAN", "TYPE-C 16PIN 2MD(073)",
     "datasheets/TYPE-C-16PIN-073.pdf", "SHOU HAN vendor drawing; contacts transcribed from drawing",
     "Gear:TBD_LAYOUT_BLOCKED", pins(
         ("A1", "GND", "GND"), ("A4", "VBUS", "USB_5V"), ("A5", "CC1", "CC1"),
         ("A6", "D+", "USB_D+"), ("A7", "D-", "USB_D-"), ("A8", "SBU1", "NC"),
         ("A9", "VBUS", "USB_5V"), ("A12", "GND", "GND"),
         ("B1", "GND", "GND"), ("B4", "VBUS", "USB_5V"), ("B5", "CC2", "CC2"),
         ("B6", "D+", "USB_D+"), ("B7", "D-", "USB_D-"), ("B8", "SBU2", "NC"),
         ("B9", "VBUS", "USB_5V"), ("B12", "GND", "GND"), ("S1", "SHIELD", "GND"))),
    ("U1", "USBLC6-2SC6", "STMicroelectronics", "USBLC6-2SC6",
     "datasheets/USBLC6-2SC6.pdf", "ST Doc ID 11265 Rev 5",
     "Package_TO_SOT_SMD:SOT-23-6", pins(
         ("1", "I/O1", "USB_D+"), ("2", "GND", "GND"), ("3", "I/O2", "USB_D-"),
         ("4", "I/O3", "USB_D-"), ("5", "VBUS", "USB_5V"), ("6", "I/O4", "USB_D+"))),
    ("R1", "5.1k", "FOJAN", "FRC0402J512TS", "docs/component-selection.md",
     "USB Type-C Specification Rd; component-selection C13", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "CC1"), ("2", "2", "GND"))),
    ("R2", "5.1k", "FOJAN", "FRC0402J512TS", "docs/component-selection.md",
     "USB Type-C Specification Rd; component-selection C13", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "CC2"), ("2", "2", "GND"))),
    ("C1", "10uF 10V X5R", "Samsung Electro-Mechanics", "CL10A106KP8NNNC",
     "docs/component-selection.md", "ME6217 input capacitor; component-selection C18",
     "Capacitor_SMD:C_0603_1608Metric", pins(("1", "1", "USB_5V"), ("2", "2", "GND"))),
    ("U2", "ME6217C33P5G", "MICRONE (Nanjing Micro One)", "ME6217C33P5G",
     "datasheets/me6217.pdf", "ME6217 datasheet, SOT-89-5 pin assignment",
     "Package_TO_SOT_SMD:SOT-89-5", pins(
         ("1", "CE", "USB_5V"), ("2", "VSS", "GND"), ("3", "NC", "NC"),
         ("4", "VIN", "USB_5V"), ("5", "VOUT", "+3V3"))),
    ("C2", "10uF 10V X5R", "Samsung Electro-Mechanics", "CL10A106KP8NNNC",
     "docs/component-selection.md", "ME6217 output capacitor; component-selection C18",
     "Capacitor_SMD:C_0603_1608Metric", pins(("1", "1", "+3V3"), ("2", "2", "GND"))),
    ("C3", "100nF 16V X7R", "Samsung Electro-Mechanics", "CL05B104KO5NNNC",
     "datasheets/esp32-c3-wroom-02_datasheet_en.pdf", "ESP32-C3-WROOM-02 datasheet v1.7 Fig. 9-1",
     "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "+3V3"), ("2", "2", "GND"))),
    ("U3", "ESP32-C3-WROOM-02-N4", "Espressif Systems", "ESP32-C3-WROOM-02-N4",
     "datasheets/esp32-c3-wroom-02_datasheet_en.pdf", "ESP32-C3-WROOM-02 datasheet v1.7 Table 3-1",
     "RF_Module:ESP32-C3-WROOM-02", pins(
         ("1", "3V3", "+3V3"), ("2", "EN", "EN_RC"), ("3", "GPIO4", "INK_RST"),
         ("4", "GPIO5", "INK_BUSY"), ("5", "GPIO6", "INK_CLK"), ("6", "GPIO7", "INK_MOSI"),
         ("7", "GPIO8", "BTN_B"), ("8", "GPIO9", "BOOT"), ("9", "GND", "GND"),
         ("10", "GPIO10", "INK_CS"), ("11", "RXD0/GPIO20", "U0_RXD"),
         ("12", "TXD0/GPIO21", "U0_TXD"), ("13", "USB_D-/GPIO18", "USB_D-"),
         ("14", "USB_D+/GPIO19", "USB_D+"), ("15", "GPIO3", "INK_DC"),
         ("16", "GPIO2", "GPIO2_STRAP"), ("17", "GPIO1", "CADENCE"),
         ("18", "GPIO0", "BTN_A"), ("19", "GND", "GND"))),
    ("R3", "10k", "FOJAN", "FRC0402F1002TS", "docs/component-selection.md",
     "ESP32-C3-WROOM-02 datasheet v1.7 section 9", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "+3V3"), ("2", "2", "EN_RC"))),
    ("C4", "1uF 10V X7R", "Yageo", "CC0402KRX7R6BB105", "docs/component-selection.md",
     "ESP32-C3-WROOM-02 datasheet v1.7 section 9", "Capacitor_SMD:C_0402_1005Metric",
     pins(("1", "1", "EN_RC"), ("2", "2", "GND"))),
    ("R4", "10k", "FOJAN", "FRC0402F1002TS", "docs/component-selection.md",
     "GPIO9 BOOT pull-up", "Resistor_SMD:R_0402_1005Metric", pins(("1", "1", "+3V3"), ("2", "2", "BOOT"))),
    ("R5", "100R", "TBD", "TBD", "docs/schematic.md",
     "Series impedance; value requires sensor transient qualification", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "CADENCE_EXT"), ("2", "2", "CADENCE"))),
    ("R6", "100k", "FOJAN", "FRC0402F1003TS", "docs/component-selection.md",
     "CADENCE pull-up; component-selection C15", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "+3V3"), ("2", "2", "CADENCE"))),
    ("R7", "1M", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "GDEY029T94 Rev 1.0 section 12, manufacturer figure R1", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "GDR"), ("2", "2", "GND"))),
    ("R8", "2.2R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "GDEY029T94 Rev 1.0 section 12, manufacturer figure R2", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "RESE"), ("2", "2", "GND"))),
    ("R9", "10k", "FOJAN", "FRC0402F1002TS", "datasheets/esp32-c3_datasheet_en.pdf",
     "GPIO8 external pull-up for ROM download strap", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "+3V3"), ("2", "2", "BTN_B"))),
    ("R10", "10k", "FOJAN", "FRC0402F1002TS", "datasheets/esp32-c3_datasheet_en.pdf",
     "GPIO2 external pull-up; avoids a GPIO-to-rail short", "Resistor_SMD:R_0402_1005Metric",
     pins(("1", "1", "+3V3"), ("2", "2", "GPIO2_STRAP"))),
    ("R11", "10k", "FOJAN", "FRC0402F1002TS", "docs/architecture.md",
     "BTN_A external pull-up", "Resistor_SMD:R_0402_1005Metric", pins(("1", "1", "+3V3"), ("2", "2", "BTN_A"))),
    ("SW1", "KH-6X6X5H-STM", "Shenzhen Kinghelm", "KH-6X6X5H-STM",
     "datasheets/KH-6X6X5H-STM.pdf", "Kinghelm drawing; BOOT button", "Gear:BUTTON_6X6",
     pins(("1", "1", "BOOT"), ("2", "2", "GND"))),
    ("SW2", "KH-6X6X5H-STM", "Shenzhen Kinghelm", "KH-6X6X5H-STM",
     "datasheets/KH-6X6X5H-STM.pdf", "Kinghelm drawing; BTN_A button", "Gear:BUTTON_6X6",
     pins(("1", "1", "BTN_A"), ("2", "2", "GND"))),
    ("SW3", "KH-6X6X5H-STM", "Shenzhen Kinghelm", "KH-6X6X5H-STM",
     "datasheets/KH-6X6X5H-STM.pdf", "Kinghelm drawing; BTN_B button", "Gear:BUTTON_6X6",
     pins(("1", "1", "BTN_B"), ("2", "2", "GND"))),
    ("J2", "XY-XH2.54-2PWT", "TBD", "TBD", "docs/component-selection.md",
     "Catalog record has no manufacturer or verified drawing; PLANNING_ONLY", "Gear:TBD_LAYOUT_BLOCKED",
     pins(("1", "SENSOR", "CADENCE_EXT"), ("2", "GND", "GND"))),
    ("D1", "PESD5V0S1BA", "UMW", "PESD5V0S1BA",
     "datasheets/PESD5V0S1BA-UMW.pdf", "UMW Jan 2025 datasheet; bidirectional terminals 1 and 2",
     "Diode_SMD:D_SOD-323", pins(("1", "P1", "CADENCE_EXT"), ("2", "P2", "GND"))),
    ("J3", "UART0 1x4 header", "TBD", "TBD", "docs/component-selection.md",
     "Exact male header not selected; PLANNING_ONLY", "Gear:TBD_LAYOUT_BLOCKED",
     pins(("1", "3V3", "+3V3"), ("2", "U0_TXD", "U0_TXD"), ("3", "U0_RXD", "U0_RXD"), ("4", "GND", "GND"))),
    ("J4", "24-pin 0.5 mm FPC", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Panel pin map from GDEY029T94; connector MPN/footprint unverified; PLANNING_ONLY", "Gear:TBD_LAYOUT_BLOCKED",
     pins(
         ("1", "NC", "NC"), ("2", "GDR", "GDR"), ("3", "RESE", "RESE"), ("4", "NC", "NC"),
         ("5", "VSH2", "VSH2"), ("6", "TSCL", "NC"), ("7", "TSDA", "NC"), ("8", "BS1", "GND"),
         ("9", "BUSY", "INK_BUSY"), ("10", "RES#", "INK_RST"), ("11", "D/C#", "INK_DC"),
         ("12", "CS#", "INK_CS"), ("13", "SCL", "INK_CLK"), ("14", "SDA", "INK_MOSI"),
         ("15", "VDDIO", "+3V3"), ("16", "VCI", "+3V3"), ("17", "VSS", "GND"),
         ("18", "VDD", "EPD_VDD"), ("19", "VPP", "NC"), ("20", "VSH1", "VSH1"),
         ("21", "VGH", "PREVGH"), ("22", "VSL", "PREVGL"), ("23", "VGL", "PREVGL"), ("24", "VCOM", "VCOM"))),
    ("L1", "47uH 500mA", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "GDEY029T94 Rev 1.0 section 12", "Inductor_SMD:L_1210_3225Metric",
     pins(("1", "1", "+3V3"), ("2", "2", "EPD_SW"))),
    ("Q1", "Si1308EDL", "Vishay Siliconix", "Si1308EDL-T1-GE3",
     "https://www.vishay.com/docs/63399/si1308edl.pdf", "Vishay document 63399 Rev C p1: SC-70/SOT-323, 1G/2S/3D; ordering information",
     "Package_TO_SOT_SMD:SOT-323_SC-70", pins(("1", "G", "GDR"), ("2", "S", "RESE"), ("3", "D", "EPD_SW"))),
    ("D2", "MBR0530", "onsemi", "MBR0530",
     "https://www.onsemi.com/pdf/datasheet/mbr0530-d.pdf", "onsemi MBR0530 datasheet; pin 1 cathode, pin 2 anode",
     "Diode_SMD:D_SOD-123", pins(("1", "K", "PUMP"), ("2", "A", "GND"))),
    ("D3", "MBR0530", "onsemi", "MBR0530",
     "https://www.onsemi.com/pdf/datasheet/mbr0530-d.pdf", "onsemi MBR0530 datasheet; pin 1 cathode, pin 2 anode",
     "Diode_SMD:D_SOD-123", pins(("1", "K", "PREVGL"), ("2", "A", "PUMP"))),
    ("D4", "MBR0530", "onsemi", "MBR0530",
     "https://www.onsemi.com/pdf/datasheet/mbr0530-d.pdf", "onsemi MBR0530 datasheet; pin 1 cathode, pin 2 anode",
     "Diode_SMD:D_SOD-123", pins(("1", "K", "PREVGH"), ("2", "A", "EPD_SW"))),
    ("C5", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C5, PREVGH reservoir", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "PREVGH"), ("2", "2", "GND"))),
    ("C6", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C6, VCI bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "+3V3"), ("2", "2", "GND"))),
    ("C7", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C7, VDD bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "EPD_VDD"), ("2", "2", "GND"))),
    ("C8", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C2, VSH2 bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "VSH2"), ("2", "2", "GND"))),
    ("C9", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C9, VSH1 bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "VSH1"), ("2", "2", "GND"))),
    ("C10", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C10, VGH bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "PREVGH"), ("2", "2", "GND"))),
    ("C11", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C11, VSL bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "PREVGL"), ("2", "2", "GND"))),
    ("C12", "1uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C12, VCOM bypass", "Capacitor_SMD:C_0402_1005Metric", pins(("1", "1", "VCOM"), ("2", "2", "GND"))),
    ("C13", "4.7uF 25V X7R", "TBD", "TBD", "datasheets/GDEY029T94.pdf",
     "Manufacturer figure C3, flying capacitor", "Capacitor_SMD:C_0603_1608Metric", pins(("1", "1", "EPD_SW"), ("2", "2", "PUMP"))),
]

for ref, net in (("PWR1", "USB_5V"), ("PWR2", "GND"), ("PWR3", "PREVGH"), ("PWR4", "PREVGL")):
    PARTS.append((ref, "PWR_FLAG", "N/A", "N/A", "KiCad:power", "ERC supply-source flag",
                  "", pins(("1", "PWR", net))))
for number, net in enumerate(("USB_5V", "+3V3", "CADENCE", "INK_CLK", "INK_MOSI", "BOOT", "U0_TXD", "GND", "EN_RC"), 1):
    PARTS.append((f"TP{number}", f"Test pad {net}", "Bare copper", "N/A", "docs/architecture.md",
                  "Zero-BOM test point", "Gear:TEST_PAD", pins(("1", "PAD", net))))


def pin_type(ref: str, number: str, name: str) -> str:
    if name == "NC":
        return "no_connect"
    if ref in {"R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11",
               "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "C10", "C11", "C12", "C13",
               "D1", "D2", "D3", "D4", "L1", "SW1", "SW2", "SW3", "J2", "J3", "J4"}:
        return "passive"
    if ref.startswith("PWR"):
        return "power_out"
    if ref.startswith("TP"):
        return "passive"
    if ref == "J1":
        return "passive"
    if ref == "U1":
        return "power_in" if name in {"GND", "VBUS"} else "bidirectional"
    if ref == "U2":
        return {"CE": "input", "VSS": "power_in", "VIN": "power_in", "VOUT": "power_out"}[name]
    if ref == "U3":
        if name in {"3V3", "GND"}:
            return "power_in"
        if name in {"EN", "GPIO5", "GPIO8", "GPIO9", "RXD0/GPIO20", "GPIO2", "GPIO1", "GPIO0"}:
            return "input"
        if name in {"USB_D-/GPIO18", "USB_D+/GPIO19"}:
            return "bidirectional"
        return "output"
    if ref == "Q1":
        return "input" if name == "G" else "passive"
    raise ValueError(f"missing electrical type for {ref}.{number} {name}")


def lib_symbol(index: int, part: tuple) -> str:
    ref, value, _, _, _, _, _, part_pins = part
    name = f"Gear:{ref}_{index}"
    h = max(5.08, len(part_pins) * 1.27 + 2.54)
    lines = [
        f'    (symbol "{name}"', '      (pin_names (offset 0))', '      (exclude_from_sim no)',
        f'      (in_bom {"no" if ref.startswith(("TP", "PWR")) else "yes"})',
        f'      (on_board {"no" if ref.startswith("PWR") else "yes"})',
        f'      (property "Reference" "{q(ref.rstrip("0123456789") or ref)}" (at 0 {h + 2.54} 0) {effects()})',
        f'      (property "Value" "{q(value)}" (at 0 {-h - 2.54} 0) {effects()})',
        '      (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))',
        '      (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))',
        '      (property "Description" "" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))',
        f'      (symbol "{ref}_{index}_0_1"',
        f'        (rectangle (start -5.08 {h}) (end 5.08 {-h}) (stroke (width 0) (type default)) (fill (type background)))', '      )',
        f'      (symbol "{ref}_{index}_1_1"',
    ]
    for pin_index, (number, pin_name, _) in enumerate(part_pins):
        y = h - 2.54 - pin_index * 2.54
        lines.append(f'        (pin {pin_type(ref, number, pin_name)} line (at -7.62 {y} 0) (length 2.54) '
                     f'(name "{q(pin_name)}" {effects(1.0)}) (number "{q(number)}" {effects(1.0)}))')
    return "\n".join(lines + ['      )', '    )'])


def instance(index: int, part: tuple) -> str:
    ref, value, manufacturer, mpn, datasheet, citation, footprint, part_pins = part
    col, row = index % 7, index // 7
    x, y = 35.56 + col * 101.6, 40.64 + row * 71.12
    h = max(5.08, len(part_pins) * 1.27 + 2.54)
    in_bom = "no" if ref.startswith(("TP", "PWR")) else "yes"
    on_board = "no" if ref.startswith("PWR") else "yes"
    # Unknown parts and the unverified button drawing must not acquire a
    # plausible-looking production land pattern merely to make ERC quiet.
    if mpn == "TBD" or ref.startswith("SW"):
        footprint = "Gear:TBD_LAYOUT_BLOCKED"
    lines = [
        f'  (symbol (lib_id "Gear:{ref}_{index}") (at {x} {y} 0) (unit 1)',
        f'    (exclude_from_sim no) (in_bom {in_bom}) (on_board {on_board})', f'    (uuid {uid(f"symbol-{ref}")})',
        f'    (property "Reference" "{q(ref)}" (at {x} {y - h - 5.08} 0) {effects()})',
        f'    (property "Value" "{q(value)}" (at {x} {y + h + 5.08} 0) {effects()})',
        f'    (property "Footprint" "{q(footprint)}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
        f'    (property "Datasheet" "{q(datasheet)}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
        f'    (property "Citation" "{q(citation)}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
        f'    (property "Manufacturer" "{q(manufacturer)}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
        f'    (property "MPN" "{q(mpn)}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
        f'    (property "Validation_Status" "UNVERIFIED_DO_NOT_BUILD" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
        f'    (property "BOM Comments" "PLANNING_ONLY; see docs/schematic.md; no live sourcing performed" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))',
    ]
    labels = []
    for pin_index, (number, _, net) in enumerate(part_pins):
        py, px = y - (h - 2.54 - pin_index * 2.54), x - 7.62
        lines.append(f'    (pin "{q(number)}" (uuid {uid(f"pin-{ref}-{number}")}))')
        if net == "NC":
            labels.append(f'  (no_connect (at {px} {py}) (uuid {uid(f"nc-{ref}-{number}")}))')
        else:
            labels.append(f'  (global_label "{q(net)}" (shape input) (at {px} {py} 0) '
                          f'(effects (font (size 1.0 1.0))) (uuid {uid(f"label-{ref}-{number}")}))')
    return "\n".join(lines + ["  )"] + labels)


def main() -> None:
    symbols = "\n".join(lib_symbol(i, part) for i, part in enumerate(PARTS, 1))
    instances = "\n".join(instance(i, part) for i, part in enumerate(PARTS, 1))
    text = f'''(kicad_sch (version 20231120) (generator eeschema)
  (uuid {uid("root")})
  (paper "A1")
  (title_block
    (title "Gear Miles - USB-C ESP32-C3 and GDEY029T94")
    (date "2026-09-14")
    (rev "Issue 3 - BLOCKED")
    (company "Gear Miles"))
  (lib_symbols
{symbols}
  )
  (text "NOT FOR FABRICATION OR POWER-UP: REJECTED CANDIDATE. Pump diode polarity and panel capacitor endpoints require correction and independent manufacturer-diagram review." (exclude_from_sim no) (at 45 555 0)
    (effects (font (size 1.27 1.27)) (justify left bottom)))
  (text "KNOWN BLOCKER: D2 A=GND/K=PUMP and D3 A=PUMP/K=PREVGL cannot form the intended negative pump. ERC and netlist tests do not validate analog operation." (exclude_from_sim no) (at 45 563 0)
    (effects (font (size 1.27 1.27)) (justify left bottom)))
  (text "VSH2 is bypassed to GND through C8 (manufacturer figure C2), never hard-grounded. PWR_FLAGs are limited to USB external rails and panel-derived PREVGH/PREVGL rails." (exclude_from_sim no) (at 45.72 571.5 0)
    (effects (font (size 1.27 1.27)) (justify left bottom)))
{instances}
  (sheet_instances (path "/" (page "1")))
)'''
    OUT.write_text(text)
    LIB.write_text("(kicad_symbol_lib (version 20231120) (generator kicad_symbol_editor)\n" f"{symbols}\n)\n")


if __name__ == "__main__":
    main()
