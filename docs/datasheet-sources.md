# Datasheet source manifest

Datasheet PDFs themselves are gitignored (re-downloadable binaries under
`datasheets/`); this manifest is the committed record of exactly which
documents every datasheet citation in `docs/architecture.md` (and later
`docs/component-selection.md`) refers to.

Retrieved: 2026-09-09 (UTC), via direct vendor/GitHub-hosted downloads.

| Document (file) | Version | SHA-256 | Used for |
|---|---|---|---|
| [esp32-c3-wroom-02_datasheet_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32-c3-wroom-02_datasheet_en.pdf) | v1.7 | 2375b1be75ebbfcffa050e6a26c3ee7957bc7171b319066081a491d1b2e21161 | Table 3-1 pin map; Tables 6-4/6-5/6-6 current consumption; EN pin note |
| [esp32-c3_datasheet_en.pdf](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf) | v2.4 | 833fc000b4b3c3d39c496fcbd597fed5806956503ce7390b19cc8ae82f19f968 | Table 3-1 strapping pins (GPIO2/8/9); Table 3-3 boot modes; Table 5-2 recommended operating conditions |
| [2.9inch_e-Paper_Datasheet.pdf](https://raw.githubusercontent.com/LilyGO/ESP32_T5Epaper_2.9inch/master/Documents/2.9inch_e-Paper_Datasheet.pdf) | waveshare-hosted doc | 618a4e9888163ffb8f2a6ba0746caa88557be5bae58ceb533842726de0b541a9 | §4 mechanical; §6.1 24-pin FPC pinout; Table 9-1 DC characteristics (VCI 2.4–3.7 V); §11 power consumption (26.4/40 mW update, ≤0.017 mW standby) |

Caveats:

- The e-paper PDF is a distributor-hosted specification for a 2.9"
  296×128 panel used as the *family reference* at baseline time. The panel
  family is EOL-flagged at the manufacturer (risk R8); issue #2 must
  confirm the final MPN and re-verify this manifest's panel rows against the
  chosen part's official datasheet before the schematic locks.
- No distributor API keys exist in this executor environment, so lifecycle
  and pricing evidence was intentionally NOT collected at baseline time —
  it belongs to issue #2 with live sourcing.
