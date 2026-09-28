# DESPI-C02 Vector Schematic Probe Summary — 2026-09-28

Evidence-only artifact. No schematic netlist, symbol properties, or blocker
gate code were changed in this step.

## Provenance
- Source: Good Display official CDN `DESPI-C02_SCH V1.0.pdf`
- Size: 468,414 bytes
- SHA-256: `b1893766d212249429876aa1cb3e7cf929712ac2f2b262ce2da39948c26128b1` (matches expected)
- Tool: `hardware/kicad/tools/probe_despi_vector_sch.py` (PyMuPDF 1.28.2)
- Output JSON: `hardware/kicad/reports/2026-09-28/despi-vector-schematic-probe.json`

## Findings in Pump Region (x: 250..900, y: 280..820)
1. **Diode glyph geometry is fully vector-rendered**:
   - D1 apex points left `(647.7, 365.8)`, cathode bar vertical at `x=645.3..647.7` (`y=342.4..389.2`).
   - D2 apex points right `(671.1, 482.8)`, cathode bar vertical at `x=671.1..673.4` (`y=459.4..506.2`).
   - D3 apex points right `(671.1, 623.1)`, cathode bar vertical at `x=671.1..673.4` (`y=599.7..646.5`).
2. **Net label corroboration**:
   - `N0PREVGL` text sits at `(689.0, 518.3)`, directly adjacent to the pin-23 (VGL) net stub.
   - `N0PREVGH` text sits at `(690.0, 692.3)`, adjacent to the D3 cathode/VGH rail stub.
3. **Corroboration of the engineering-topology blockers**:
   - In DESPI-C02 V1.0, the positive booster rail rectifier points cathode-right toward the rail, and the negative clamp/rectifier pair points in the complementary direction required to pump negative charge.
   - This provides independent, vector-level corroboration from Good Display's reference design that the candidate netlist's reversed D2/D3 orientations (`NEGATIVE_PUMP_CLAMP` and `NEGATIVE_PUMP_RECTIFIER`) are indeed inverted.

## Remaining Blocker Status for Issue #3
The PR remains a **DRAFT** (`gh pr view 12` state `OPEN`, `isDraft=true`, `mergeStateStatus=UNSTABLE`).
The blocker gate `check_blockers.py` continues to exit with code **3** across five groups:
1. `NEGATIVE_PUMP_CLAMP` (D2 polarity)
2. `NEGATIVE_PUMP_RECTIFIER` (D3 polarity)
3. `MISSING_PART_IDENTITY` (17 items)
4. `PLACEHOLDER_FOOTPRINT` (21 items)
5. `DECLARED_UNVERIFIED` (41 items)

This pass records the vector schematic probe and evidence summary without unblocking or merging unverified hardware.
