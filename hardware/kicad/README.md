# KiCad hardware source

`gear-miles.kicad_sch` is an editable KiCad 9 schematic generated
deterministically by `tools/generate_schematic.py`. The project-local embedded
symbols preserve the current candidate pin map rather than depending on host
library versions. **This candidate failed electrical review; do not build or
power it.** Zero ERC and candidate-map tests are not circuit validation.

Regenerate, export its netlist, and run ERC with:

```sh
python3 hardware/kicad/tools/generate_schematic.py
kicad-cli sch export netlist --format kicadxml \
  -o hardware/kicad/reports/gear-miles.net hardware/kicad/gear-miles.kicad_sch
kicad-cli sch erc --exit-code-violations --format json -o hardware/kicad/reports/erc.json \
  hardware/kicad/gear-miles.kicad_sch
```

The schematic uses an A1 landscape grid so all bodies and annotations fit on a
single page. `tests/test_netlist.py` checks complete physical references and
critical exact maps, including a deliberate failing mutation. Run
`tests/test_schematic_bounds.py` for source bounds. The panel connector and
the manufacturer-circuit transcription are fabrication blockers documented in
`docs/schematic.md`.
