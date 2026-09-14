# Datasheet source manifest

## 2026-09-14 implementation review additions

The legacy rows below are retained, not blanket-revalidated. Actual local
manufacturer PDFs were read with `pdftotext`/PyMuPDF this run:

| Document | Retrieval/source | SHA-256 | Evidence |
|---|---|---|---|
| Si1308EDL | [Vishay PDF](https://www.vishay.com/docs/63399/si1308edl.pdf), downloaded 2026-09-14 | `a86d6f766f259d8b170a01081f577055f37b527257a0f9af497c8f2c33625c9a` | Document 63399 Rev C p1: **SOT-323/SC-70**, 1 gate, 2 source, 3 drain; orderable Si1308EDL-T1-GE3. Not SOT-23. |
| MBR0530 | [onsemi PDF](https://www.onsemi.com/pdf/datasheet/mbr0530-d.pdf), downloaded 2026-09-14 | `11459fe319b080be6b12cb596d20f96c1d23381e5e9f40ff017afcca0d187a23` | MBR0530/D Rev 2 p1, SOD-123, 30 V / 500 mA. Physical marking/direction and full pump still require review. |
| UMW PESD5V0S1BA/BB/BL | Existing `datasheets/PESD5V0S1BA-UMW.pdf`, read 2026-09-14 | `58082d1cca5eefb1d872f35948be198ab5a86f0f83216e07f2832d399f27d00f` | Jan 2025, UTD Semiconductor Co. Limited / UMW, p1: bidirectional, clamp 14 V, component-level ESD >30 kV. This does **not** establish safe ESP32 pin clamping. |
| Kinghelm FPC drawing | Existing `datasheets/KH-FG0.5-H2.0-24PIN.pdf`, read 2026-09-14 | `288bd7abe5351949830d2c392e814da3db04397699d1562a0d36462827c15c85` | Text identifies Kinghelm/KH-FG0.5-H2.0-24PIN, not XUNPU. Cannot validate the selected XUNPU connector. |

No structured extraction cache, lifecycle audit, or new live distributor
pricing/stock validation was completed. See [schematic blockers](schematic.md).

## Legacy manifest

Datasheet PDFs themselves are gitignored (re-downloadable binaries under
`datasheets/`); this manifest is the committed record of exactly which
documents every datasheet citation in `docs/architecture.md` and
`docs/component-selection.md` refers to.

Retrieved: 2026-09-09 (UTC) for the baseline rows; **updated 2026-09-11
(UTC)** by issue #2 component selection (new rows + re-verification).

| Document (file) | Version | SHA-256 | Used for |
|---|---|---|---|
| [esp32-c3-wroom-02_datasheet_en.pdf](https://documentation.espressif.com/esp32-c3-wroom-02_datasheet_en.pdf) | v1.7 | 2375b1be75ebbfcffa050e6a26c3ee7957bc7171b319066081a491d1b2e21161 | Table 3-1 pin map; Tables 6-4/6-5/6-6 current consumption; EN RC guidance (§9); recommended operating VDD33. Re-downloaded 2026-09-11: SHA-256 unchanged from baseline row |
| [esp32-c3_datasheet_en.pdf](https://documentation.espressif.com/esp32-c3_datasheet_en.pdf) | v2.4 | 833fc000b4b3c3d39c496fcbd597fed5806956503ce7390b19cc8ae82f19f968 | Table 3-1 strapping pins (GPIO2/8/9); Table 3-3 boot modes; Table 5-2 recommended operating conditions. Re-downloaded 2026-09-11: SHA-256 unchanged |
| [GDEY029T94.pdf](https://files.seeedstudio.com/wiki/Other_Display/29-epaper/GDEY029T94.pdf) | Rev 1.0, 2021-03-15 (Dalian Good Display official; Seeed mirror) | 2ea221896f9b0463e7f53b6a4d0dff967728f2049ca7667d45878c1ff4e876a9 | §3 mechanical (36.7×79.0×1.2 mm); §5 24-pin I/O assignment (BUSY/RES#/D/C#/CS#/SCL/SDA/VDDIO/VCI/VSS/VDD + booster set); §6.1 abs-max (VCI ≤ 4.0 V); §6.2 DC/AC (3 mA typ, 0.3 s partial); §5 Note 5-5 BS1 = 4-line SPI |
| [me6217.pdf](https://jlcpcb.com/api/file/downloadByFileSystemAccessId/8586203361498161152) | ME6217 series DS (MICRONE; JLC-hosted) | 80eaafe634c50d5436a3b5cbd3a563261a8f460d8d144dbb3fa2e7ffa71b0dfa | 800 mA; dropout 100/180 mV typ/max @300 mA (3.0–5.5 V parts); VIN 2.0–6.5 V, abs-max 7.0 V; PD 1 W SOT-89-5; pinout CE/VSS/NC/VIN/VOUT; thermal shutdown 160 °C |
| [ap2112.pdf](https://www.diodes.com/assets/Datasheets/AP2112.pdf) | DS39724 Rev 2, June 2017 (Diodes) | ef8d376f2ec356e29172eb9e053819a0ebdcc576dba7fc9ab0505c568427920f | 600 mA min; AP2112-3.3 dropout 250 mV typ/400 max @600 mA; IQ 55 µA; abs-max VCC 6.5 V, Tj 150 °C; θJA 184 (SOT25)/120 (SOT89-5) °C/W — basis of Finding 1 |
| [usblc6.pdf](https://jlcpcb.com/api/file/downloadByFileSystemAccessId/8579714554416455680) | Doc ID 11265 Rev 5, Oct 2011 (ST) | 8ba7ab4ec1781b7f6052f5999a2304bd611c0ed2b5e8f36ffa5092552932a42e | Pinout I/O1,GND,I/O2,I/O3,VBUS,I/O4; IEC 61000-4-2 L4 guaranteed at device level; VBUS protection |
| [pesd5v0s1ba.pdf](https://www.nexperia.com/documents/data-sheet/PESD5V0S1BA.pdf) | 2024 revision (Nexperia family DS PESD5V0S1Bx) | 6546e415b8885ea3790c91629034c6b4e5c6961d5fde734e55dfa479dd8dc49a | IEC 61000-4-2 L4, >30 kV contact; VRWM 5 V; E3 sensor-line ESD choice (family reference; UMW-marked SKU caveat in component-selection) |
| [reed-59170.pdf](https://jlcpcb.com/api/file/downloadByFileSystemAccessId/8588911183105966080) | Littelfuse 59170 + 57045 actuator, rev 2016-03-03 | 9b0409314c668a7579dc973bf9bfa5b7e54fba007667534e4b3b0dfd38f82261 | Electrical ratings (10 W, 200 Vdc/0.5 Adc, −40…+125 °C); sensitivity options S/T/U with activate distance 6.5/5.0/4.6 mm; operate ≤1 ms; ordering code; "actuator sold separately" |
| [usbc.pdf](https://jlcpcb.com/api/file/downloadByFileSystemAccessId/8756418619037634560) | SHOU HAN vendor drawing (TYPE-C 16PIN 2MD(073)) | fe10da58945c9782ffe489b2079893a9baf90ff234efa4906e5e02744735e8f5 | Footprint/dimensions; text layer Chinese-only → pin numbering cross-checked vs USB-C standard A/B numbering + industry reference drawing (GCT USB4125-class) |
| [fpc24.pdf](https://jlcpcb.com/api/file/downloadByFileSystemAccessId/8588945046850703360) | XUNPU vendor drawing (FPC-05F-24PH20) | 6a9542752ceb7f82784b86ffd684487e4bc84eb51626d020c23b4006c4571283 | 24P/0.5 mm bottom-contact geometry (PDF needs repair — xref errors; use viewer) |
| [button-kh.pdf](https://jlcpcb.com/api/file/downloadByFileSystemAccessId/8588919340461015041) | Kinghelm KH-6X6X5H-STM 1-page drawing | 2701014ea5ea3e0f7cdd71a28fd5b766cbf4eca1c2f79cce78284fdeb6831e4f | Mechanical drawing only; electrical ratings from catalog attributes (gap) |

## Withdrawn baseline row (superseded)

The baseline-era panel reference `2.9inch_e-Paper_Datasheet.pdf`
(waveshare-hosted family doc, SHA-256
618a4e9888163ffb8f2a6ba0746caa88557be5bae58ceb533842726de0b541a9, retrieved
2026-09-09) described the **EOL-flagged** panel family (risk R8) and is
**withdrawn as a design-reference document**. It is replaced by the
official GDEY029T94 datasheet above. The architecture §4/§2 figures that
cited it are updated: panel typ update current is now **3.0 mA @ 3.0 V
(typ), deep sleep 1–5 µA** (GDEY029T94 §6.2) instead of the old
26.4/40 mW figure, and the "SSD1680-class" claim is withdrawn — the
selected panel's datasheet does not name its driver IC.

## Caveats

* JLC/LCSC catalog lifecycle attributes (`Status: Active`) on the
  component rows are snapshot-backed (`fetched_live: false`) — no
  DigiKey/Mouser API keys exist in this executor environment, so
  independent live lifecycle + pricing evidence was not retrieved.
* ME6217 datasheet does not publish θJA — thermal adequacy argument in
  component-selection rests on PD = 1.0 W (SOT-89-5) + copper design, to be
  validated at layout (#5) and bench (#8).
* `fpc24.pdf` has a broken xref table; `pesd-techpublic.pdf` (UMW-marked
  PESD5V0S1BA, 2.2 MB) extracted no text — both need viewer/manual
  re-verification before fab (#9).
