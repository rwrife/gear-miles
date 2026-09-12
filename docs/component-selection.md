# Gear Miles — Component Selection (issue #2)

Status: **selected with datasheet evidence** (issue #2). Every MPN below is
justified by an exact datasheet citation; every distributor stock/price figure
is live evidence retrieved on **2026-09-11** from the JLCPCB/LCSC catalog via
the jlcsearch community API (no DigiKey/Mouser keys exist in this executor
environment — see Gaps). Datasheet documents backing each citation are
recorded in [`datasheet-sources.md`](datasheet-sources.md) with SHA-256.
No value here is a bench measurement.

Requirement IDs (E1–E6, M1–M4) refer to `hardware/requirements.md`.
Decision points Q1–Q6 refer to `docs/architecture.md` §8.

## Decision summary

| ID | Function | Manufacturer | MPN | LCSC # | Tier |
|---|---|---|---|---|---|
| C1 | Wi-Fi MCU module | Espressif Systems | ESP32-C3-WROOM-02-N4 | C2934560 | Extended |
| C2 | 3.3 V LDO | MICRONE (Nanjing Micro One) | ME6217C33P5G | C81592 | Extended |
| C2-alt | LDO second source (Diodes die) | Diodes Incorporated | AP2112R5A-3.3TRG1 | C5375641 | Extended |
| C3 | E-ink panel (Q1) | Dalian Good Display | GDEY029T94 | not cataloged | panel |
| C4 | Panel FPC connector | XUNPU | FPC-05F-24PH20 | C2856805 | Extended |
| C5 | Cadence sensor (Q2) | Littelfuse | 59170-1-S-00-D | C315909 | Extended |
| C6 | Crank magnet | TBD | 57045-class actuator (sold separately per DS) | not found | TBD |
| C7 | USB-C receptacle | SHOU HAN | TYPE-C 16PIN 2MD(073) | C2765186 | Extended |
| C8 | USB D+/D−/VBUS ESD | STMicroelectronics | USBLC6-2SC6 | C7519 | Extended |
| C9 | Sensor-line ESD | UMW | PESD5V0S1BA | C5158048 | Extended |
| C10 | Buttons ×2 (+BOOT) | Shenzhen Kinghelm | KH-6X6X5H-STM | C2837531 | Extended |
| C11 | Sensor header (shrouded) | (blank in catalog record; "XY" series) | XY-XH2.54-2PWT | C52190561 | Extended |
| C12 | UART0 log header | TBD | 2.54 mm 1×4 straight male | TBD | TBD |
| C13 | CC1/CC2 Rd straps ×2 | FOJAN | FRC0402J512 TS (5.1 kΩ ±5%) | C2906948 | Extended |
| C14 | Pull-ups (BOOT, spare) | FOJAN | FRC0402F1002TS (10 kΩ ±1%) | C2906861 | Extended |
| C15 | CADENCE pull-up | FOJAN | FRC0402F1003TS (100 kΩ ±1%) | C2906859 | Extended |
| C16 | EN RC: R | FOJAN | FRC0402F1002TS (10 kΩ ±1%) | C2906861 | Extended |
| C17 | EN RC: C + panel VDD | Yageo | CC0402KRX7R6BB105 (1 µF 10 V X7R) | C7500381 | Extended |
| C18 | Bulk caps (USB, LDO out, panel VCI) | Samsung Electro-Mechanics | CL10A106KP8NNNC (10 µF 10 V X5R) | C19702 | **Basic** |
| C19 | Bypass caps | Samsung Electro-Mechanics | CL05B104KO5NNNC (100 nF 16 V X7R) | C1525 | **Basic** |
| C20 | Test points | — | bare exposed copper pads + silkscreen | — | — |

Quantities and schematic wiring land in issue #3 (this issue selects parts and
evidence, not the schematic).

## Part-by-part justification

### C1 — ESP32-C3-WROOM-02-N4 (module)

* Requirement: Wi-Fi 2.4 GHz b/g/n, PCB antenna, native USB CDC, ≥ 4 MB flash,
  3.0–3.6 V supply (E1/E2), 70×50 mm board budget (M1).
* Datasheet: *ESP32-C3-WROOM-02 & WROOM-02U Datasheet v1.7* — operating
  voltage 3.0–3.6 V (§2/§6, "Operating voltage/Power supply: 3.0 ~ 3.6 V";
  Table 6-1 VDD33 recommended 3.0–3.6 V, p. ~22); Wi-Fi TX peak 345 mA
  (Table 6-4, p. 23 — the E1 revision basis); pin map Table 3-1 (pp. 10–11)
  matches the frozen GPIO map in `architecture.md` §3 exactly (pin 1 = 3V3,
  pin 2 = EN "Do not leave the EN pin floating", pins 5/6/7/8 = GPIO6/7/8/9,
  pin 13/14 = GPIO18/19 = USB_D−/USB_D+, pins 11/12 = U0RXD/U0TXD).
* Symbol check: the candidate Espressif KiCad library symbol
  `Espressif:ESP32-C3-WROOM-02` was extracted pin-by-pin from the `.kicad_sym`
  and verified **against the datasheet Table 3-1, not another library**:
  1→3V3, 2→EN, 3→GPIO4, 4→GPIO5, 5→GPIO6, 6→GPIO7, 7→GPIO8, 8→GPIO9,
  9→GND, 10→GPIO10, 11→GPIO20/U0RXD, 12→GPIO21/U0TXD, 13→GPIO18/USB_D−,
  14→GPIO19/USB_D+, 15→GPIO3, 16→GPIO2, 17→GPIO1, 18→GPIO0, 19→GND.
  No mismatch with the datasheet or the architecture GPIO map.
* Lifecycle: catalog record (2026-09-11) lists Status **Active**
  (`fetched_live: false` — snapshot-backed listing; see Gaps).
* Sourcing evidence (retrieved 2026-09-11): LCSC C2934560, live stock 3 702,
  price US$3.2912 @1, $2.6925 @30, $2.4225 @100 (API tier list).
  The U.FL variant C2926676 (ESP32-C3-WROOM-02U-N4, stock 5 471) is **not**
  selected — M4 wants the on-board PCB antenna, no coax.
* Note: module datasheet revision on Espressif's site remains v1.7 (SHA-256
  identical to the baseline manifest row — re-verified 2026-09-11).

### C2 / C2-alt — 3.3 V LDO (Q3)

Requirement (Q3 + E1 rev A + E2): ≥ 500 mA class, ≤ 500 mV dropout at
500 mA, small package, thermal check. Load model from `architecture.md` §4:
345 mA worst-case module TX peak (rated 100 % duty in the module DS),
panel update ≈ 3 mA typ, total ≤ ~360 mA.

**Finding 1 (thermal, drives the decision).** Diodes AP2112K-3.3TRG1
(SOT-23-5, DS39724 Rev. 2): electrical characteristics fit the rail —
guaranteed 600 mA min, dropout 250 mV typ / 400 mV max @ 600 mA
(AP2112-3.3 section), Iq 55 µA typ, VIN max 6.0 V (abs-max 6.5 V),
stable with 1 µF. **But** its datasheet θJA (SOT25) = **184 °C/W**
(Absolute Maximum Ratings table). Dissipation at full 345 mA sustained
(100 % duty worst case) is (5.0 − 3.3) V × 0.345 A ≈ 0.59 W → ΔT ≈ 108 °C;
Tj ≈ 148 °C at 40 °C worst-case ambient, at/over the +150 °C junction
limit. At 25 °C ambient it is marginal (≈ 133 °C). The Q3 "≤ 500 mV dropout
at 500 mA" also cannot be demonstrated from the DS: dropout is characterized
only up to 600 mA on the SOT89-5 variant table. Conclusion: the SOT-23-5
AP2112K is rejected for sustained-worst-case margin; the decision below
uses θJA and dropout evidence, and the SOT-23-5 form factor is a documented
deviation from Q3's "SOT-223/SOT-23-5" wording (SOT-89-5 has ~the same
footprint class as SOT-23-5 with far better thermal headroom).

**Selected: ME6217C33P5G (SOT-89-5), MICRONE.** Datasheet (JLC-hosted
datasheet retrieved 2026-09-11): 800 mA max output (VIN ≥ VOUT+1.0 V
condition), dropout 100 mV typ / 180 mV max @ 300 mA for 3.0–5.5 V parts
(Electrical Characteristics), VIN range 2.0–6.5 V (abs-max VIN 7.0 V —
USB 5 V ±5 % = 5.25 V max is inside), Iq 100 µA typ, thermal shutdown
160 °C, package power dissipation PD = 1000 mW (SOT-89-5). Pinout
(SOT-89-5): 1 CE / 2 VSS / 3 NC / 4 VIN / 5 VOUT — CE tied to VIN.
Worst-case dissipation 0.59 W < 1.0 W PD; with DS-θJA unpublished (Gaps),
thermal adequacy rests on PD + copper-pour design guidance to be verified
on the #5 layout (thermal vias under the tab) and measured at #8.
At the realistic duty cycle of a 1 Hz-update idle device, dissipation is
≲ 0.02 W (13 mA modem-sleep idle ⇒ (1.7 V × 13 mA)).
Dropout spec at 300 mA (max 180 mV) supports E2 margin at 345 mA but is
**not characterized at 500 mA** — recorded gap, acceptable because the
board never draws 500 mA continuous (E1's 500 mA is a *capability* floor;
the LDO must not current-limit before 360 mA, and its 800 mA rating covers
that with margin).

C2-alt (drop-in second source, same electrical role, published θJA):
**AP2112R5A-3.3TRG1** (SOT-89-5): same DS39724 die — 600 mA min, dropout
250 mV typ @ 600 mA, θJA (SOT89-5) = 120 °C/W → Tj ≈ 111 °C at 40 °C
ambient under the 0.59 W worst case (inside +150 °C). Catalog evidence
2026-09-11: C5375641, stock 2 — JLC assembly availability is the reason it
is the alternate, not the pick. ME6217 stock is 11 127.

Rejected: AMS1117-3.3 (C6186/C347222, SOT-223) — datasheet dropout is
specified 1.1 V typ @ 800 mA (≤ ~1.2 V @ 500 mA), which **fails Q3's
≤ 500 mV @ 500 mA gate**; also 5 V−3.3 V − 1.1 V dropout at 350 mA pushes
headroom to 0.6 V — marginal for a 345 mA 100 % duty case.

### C3 — E-ink panel, Q1 resolved

**Selected: GDEY029T94 (Dalian Good Display)** — 2.9 ", 296×128, b/w,
4-wire SPI. Official DS (Rev 1.0, 2021-03-15, Good Display; mirror: Seeed CDN):

* 24-pin FPC signal set matches the frozen baseline interface contract one
  for one: §5 Input/Output Pin Assignment — 9 BUSY, 10 RES#, 11 D/C#,
  12 CS#, 13 SCL, 14 SDA, 15 VDDIO (= tie to VCI, per DS remark), 16 VCI,
  17 VSS, 18 VDD (external cap to VSS), plus booster pins GDR(2), RESE(3),
  VSH2(5), VSH1(20), VGH(21), VSL(22), VGL(23), VCOM(24), temp-sensor I2C
  TSCL/TSDA (6/7), BS1(8), NC/VPP(1,4,19). The architecture §2 "panel-family
  common signal set" is satisfied exactly.
* VCI/IO: DC characteristics at VCI = 3.0 V; VIH ≥ 0.8·VCI, VIL ≤ 0.2·VCI →
  2.4/0.6 V thresholds are ESP32-C3 3.3 V logic compatible (E5: no pin
  exceeds panel abs-max VCI + 0.5 V; VCI ≤ 4.0 V abs max).
* Power: typ operating current 3.0 mA @ 3.0 V, deep sleep 1–5 µA
  (§6.2 table) — consistent with (better than) the §4 budget line
  (8.8/13.3 mA figure was for the old family; GDEY029T94's own 3 mA typ is
  the new design input; refresh time full 3 s / fast 1.5 s / partial 0.3 s).
* **Driver IC note (honesty per metric-honesty policy):** the official DS
  does not name its controller IC ("integrated circuits including gate
  driver, source driver, MCU interface, timing controller…", §1); the
  baseline's "SSD1680-class" wording must **not** be carried forward as a
  fact for this panel — firmware in #6 drives it via its command set
  (BS1=L = 4-line 8-bit SPI, §5 Note 5-5 / Table).
* Lifecycle: the EOL-flagged family (baseline caveat) is not the selected
  part; GDEY029T94 has an active product page
  (good-display.com/product/389.html, retrieved 2026-09-11) and the panel
  is sold by the maker's own shop (buyepaper.com). **Not in the
  JLC/LCSC catalog** (searches for panel terms return only paper
  capacitors, 2026-09-11) → panel is a separate buy; #5 must verify FPC
  geometry against the mechanical drawing (§4 of DS: 36.7 × 79.0 × 1.2 mm)
  against connector C4, and #8 does the physical fit test (M2).
* **Finding 2 (environmental margin):** panel TOPR = 0…+50 °C covers our
  0…40 °C indoor requirement, but TSTG = −25…+70 °C with *optimal* storage
  23 ± 2 °C and the DS reliability program (−25↔+70 cycling) — our
  storage spec −10…+50 °C is inside absolute limits but the DS discourages
  long-term storage away from ~23 °C. Recorded as an assembly/storage
  note, not a blocker.
* Alternatives considered (R8 ≥ 2 candidates): (a) baseline family
  (DE029-910-class waveshare doc) — EOL-flagged at manufacturer, rejected
  as primary but its 24-pin map is what the contract was written against,
  so GDEY029T94 is contract-compatible by construction; (b) GDEY029T94
  variants T01/FT01/FL03 (touch/frontlight) — rejected, extra unneeded FPC
  signals and cost. Drop-in re-check stays open per R8 until fab (#9).

### C4 — Panel FPC connector

**FPC-05F-24PH20 (XUNPU), C2856805** — 24 P, 0.5 mm pitch, **bottom
contact**, 0.3 mm FFC thickness, hinged lid, 50 V / 0.5 A, −25…+85 °C
(catalog attributes + JLC drawing PDF). Matches the panel family FPC
(24-pin, 0.5 mm, contacts facing board). **Verification task recorded
for issue #5:** overlay panel DS mechanical drawing (GDEY029T94 §4) on this
connector footprint — the panel DS text does not state FPC pitch/contact
side explicitly (only the drawing), so task "M2-overlay" (issue #5) must
close this visually with the PDF. Price $0.1104 @1 / $0.0768 @150, stock
93 037 (2026-09-11).

### C5 / C6 — Cadence sensor and magnet, Q2 resolved

**Selected: reed switch, Littelfuse/C&K 59170 (LCSC 59170-1-S-00-D),
C315909.** Justification vs the Hall alternative:

* **Zero standby power** — "No standby power requirement" (DS feature
  list). An A3144-class Hall needs its supply rail live and draws ~2–6 mA
  continuously → +6…18 mA on the 3.3 V rail at all times, which would blow
  the E1 average ≤ 150 mA budget by a large multiple and defeats the
  modem-sleep strategy. Reed wins decisively on power (Q2 gap-power axis).
* E3 fit: normally-open contact, pull-up resistor on a 2-pin header line —
  open-drain-equivalent behavior the architecture §2 contract assumes.
* Gap spec for M3: sensitivity option **S** (pull-in 10–15 AT), activate
  distance **6.5 mm average** with the matched 57045 actuator (DS
  "Sensitivity Options" table; option D = gull-wing tape & reel — the LCSC
  `-D` suffix). Operate ≤ 1.0 ms / release ≤ 0.5 ms; contact ratings
  200 Vdc / 0.5 Adc switching, 10 W max — a 3.3 V / 100 kΩ pull-up sees
  ~33 µA, ~4 orders inside rating. Bounce: firmware debounce (30 ms
  window, architecture §6) covers the ≤ 1 ms operate-bounce spec.
  200 RPM (200 Hz, 400 edges/s) vs 1 ms operate time — comfortable.
* Temperature: −40…+125 °C operating (DS electrical ratings) — covers spec.
* **C6 magnet:** the 57045 actuator is "sold separately" (DS note) and no
  LCSC listing was found (2026-09-11) → MPN TBD. Interim: pair the
  10–15 AT option-S switch with a small diametric/dog-bone magnet sized
  by a printed mock and verified at bring-up (M3 fit test across ≥ 2 crank
  styles) — this is exactly the M3 verification method already in the
  requirements. No invented part number.
* Hall alternative recorded (Q2 evidence): A3144 class, catalog example
  C18221460 (A3144 SOT-23, "3.8V~40V, 4mT, 6mA, open-collector") — usable
  electrically but rejected on the continuous-current budget above.
* E3 ESD on this line: see C9.

### C7 — USB-C receptacle

**TYPE-C 16PIN 2MD(073), SHOU HAN, C2765186** — mid-mount SMT, 16 P
(both CC pins present: CC1 = A5, CC2 = B5 per USB-C standard pin numbering
which this 16-pin full-featured footprint carries; 16-pin = A1–A12 + B1+B5+B6+B7 — vendor drawing is Chinese-language PDF, #3 should overlay the vendor drawing page),
3 A power / 5 V rated, −25…+85 °C, 5 000 cycles (catalog attributes +
vendor drawing PDF, retrieved 2026-09-11). UFP wiring per architecture §2:
5.1 kΩ Rd straps on CC1/CC2 (C13), VBUS = A4/B4, GND = A1/B12, D+ = A6/B6,
D− = A7/B7. E5: nothing on this connector exceeds GPIO abs-max — D+/D− are
clamped by C8 before reaching the module. **Pinout verification method:**
USB Type-C spec pin numbering + vendor drawing; the GCT USB4125-30-B-16
drawing was used as the canonical cross-check reference (industry
reference drawing for this footprint class) — recorded here since the
vendor PDF text layer is Chinese-only (extraction gap, Gaps).
Stock 1 171 811, $0.0742 @1 (2026-09-11).

### C8 — USB data ESD

**USBLC6-2SC6, STMicroelectronics, C7519** (SOT-23-6L). ST DS
"Doc ID 11265 Rev 5" (retrieved 2026-09-11): pinout 1 I/O1, 2 GND,
3 I/O2, 4 I/O3, 5 VBUS, 6 I/O4; protects VBUS + two differential pairs;
IEC 61000-4-2 **level 4 guaranteed at device level** (contact discharge;
VPP 15 kV table); VRWM 5.25 V (catalog attribute) covers USB 5 V;
line capacitance ~3.5 pF typ — USB 2.0 FS (full-speed CDC) tolerant.
Satisfies E3/E5 for D+/D−. Stock 36 380, $0.1639 @1. Note: LCSC also
lists two other USBLC6-2SC6-branded SKUs (C2687116, C2827654) — generic
markings from other vendors; the **ST**-branded C7519 is chosen because its
datasheet is the ST Rev 5 document above (avoid brand/datasheet mismatch).
Wiring: VBUS→pin 5, D+→one I/O pair leg, D−→the other, per DS application.

### C9 — Sensor-line ESD (E3)

**PESD5V0S1BA, UMW, C5158048** — bidirectional, VRWM 5 V, VBR ≥ 7 V,
IEC 61000-4-2 + 61000-4-5 per catalog attributes; Nexperia datasheet for
the PESD5V0S1Bx family (2024 revision, retrieved 2026-09-11) lists
**IEC 61000-4-2 level 4 (ESD), VESD > 30 kV (contact)** in the
characteristics — exactly the E3 "IEC 61000-4-2 contact-level component
choice per datasheet" requirement. Clamp well above the 100 kΩ pull-up
rail (3.3 V) and below GPIO abs-max damage thresholds via series impedance.
**Brand caveat recorded:** C5158048 is UMW-marked; the Nexperia PDF is the
family reference. #3/#9 should re-confirm the UMW datasheet before fab;
if UMW's copy diverges, TECH PUBLIC C2827694 (same MPN, stock 1.06 M) is
the alternate. 5 V VRWM is also a spare choice for VBUS-side protection
if ever needed (E6: no > 24 V nodes — all parts here ≤ 6.5 V rated).

### C10 — Buttons

**KH-6X6X5H-STM, Shenzhen Kinghelm, C2837531** ×2 (+ optional BOOT reuse)
— 6×6 mm SMD gull-wing, SPST, 12 V / 50 mA, 80 000 cycles (catalog
attributes). The LCSC datasheet file is a 1-page Chinese mechanical drawing
with no extractable electrical text (Gaps) — acceptable for a tactile
switch; ratings come from the catalog attributes and the electrical duty is
trivial (GPIO-to-GND, firmware debounce per architecture §2). Part choice
will be re-checked in #3 for KiCad footprint (SW_Push + 6x6x5 footprint)
and in #9 before fab.

### C11/C12 — Headers

* Sensor header: **XY-XH2.54-2PWT, C52190561** — 2-pin shrouded
  wire-to-board, **pitch 2.54 mm per catalog attribute**, XH-series
  compatible, 250 V / 3 A rated, SMT right-angle, −40…+85 °C, UL94V-0
  (catalog attributes, fetched_live true on 2026-09-11). The LCSC record's
  manufacturer field is blank → MPN carries the "XY" vendor prefix only;
  do not attribute to a named brand without a drawing (Gaps). Keyed/shrouded
  per architecture §2; 3 A ≫ 33 µA signal.
* UART0 log header: 2.54 mm 1×4 straight male (3V3/U0_TXD/U0_RXD/GND).
  Catalog keyword search on 2026-09-11 returned only *female* 1×4 variants
  as top hits; exact male part number **TBD (→ #3/#9)** — deliberately not
  guessed. Test-point alternative already used: bare pads (C20).

### C13–C19 — Passives (values per datasheet rules, MPNs per catalog)

* **CC straps (C13):** USB Type-C spec UFP Rd = 5.1 kΩ (compliant USB-C
  receptacle default); FRC0402J512 TS 5.1 kΩ ±5 % (catalog attr; USB
  spec allows ±10 % for Rd) ×2 → 14.88 mA total from VBUS, within the
  500 mA sink advertisement.
* **BOOT pull-up (C14) / EN series R (C16):** 10 kΩ. Module DS §9
  peripheral schematic guidance: EN RC delay "usually R = 10 kΩ and
  C = 1 µF" (Fig. 9-1 section text, retrieved v1.7).
* **EN cap (C17):** 1 µF 0402 X7R 10 V — CC0402KRX7R6BB105 (Yageo,
  resolved 2026-09-11: stock 175 923, $0.0238 @1). 10 V rating on a 3.3 V
  node = 3× derating. Same part also covers panel VDD-to-VSS cap (DS §5
  pin 18 remark "a capacitor should be connected"; DS schematic figure
  value not text-extractable — default 1 µF, X7R; flagged for #5 check vs
  the DS reference-circuit figure).
* **CADENCE pull-up (C15):** **100 kΩ**, chosen at the high end of the
  E3 10–100 kΩ window to keep the ~33 µA standby current invisible in the
  E1 average budget (architecture §4 line "CADENCE pull-up 100 kΩ ≈ 33 µA");
  reed leakage/bounce are irrelevant at this impedance. Spare 10 kΩ
  populated DNP-style? No — single value populated; the requirement's
  option range stays open for #8 tuning by pad-swap.
* **Bulk (C18):** CL10A106KP8NNNC 10 µF X5R 10 V 0603 — **JLC basic** part,
  stock 5.3 M. 10 V rating on a 5 V VBUS rail (derating adequate for
  X5R ≥ 50 % margin), module reference Fig. 9-1 itself uses 10 µF + 0.1 µF
  on 3V3 (text extraction line "10uF 0.1uF") — the LDO input/output bulk
  and panel VCI follow the same 10 µF practice.
* **Bypass (C19):** CL05B104KO5NNNC 100 nF X7R 16 V 0402 — basic,
  stock 16.4 M; module DS Fig. 9-1 0.1 µF; every IC/panel rail gets one.
* Test coverage: test points are **bare copper pads + silkscreen**
  (architecture §2 list: 5V, 3V3, CADENCE, INK_CLK, INK_MOSI, BOOT,
  U0TXD, GND) — zero-BOM parts; no TP component purchased.

## E1–E6 / M1–M4 cross-check table

| Req | Where satisfied | Evidence class |
|---|---|---|
| E1 | C2 (≥ 500 mA class: 800 mA rating), CC straps C13 advertise 500 mA sink | DS + catalog |
| E2 | C2 dropout 180 mV max @ 300 mA; bulk C18 per module DS Fig. 9-1 | DS (300 mA condition only — gap noted) |
| E3 | C15 pull-up window, C9 IEC 61000-4-2 L4 DS, C11 keyed header | DS + catalog |
| E4 | (firmware/#6) + C5 ≤ 1 ms operate vs 200 Hz | DS |
| E5 | C8 clamps D±; C5 contact ratings ≫ applied; C7 pin duty ≤ ratings | DS |
| E6 | max node 5.25 V; no battery/mains parts anywhere above | all DS |
| M1 | module 31.6×19.5×3.2 + panel 36.7×79.0 (separate bezel), LDO SOT-89-5 | DS |
| M2 | GDEY029T94 DS §3/§4 + C4 24P/0.5 mm; visual overlay task → #5 | DS (drawing check open) |
| M3 | C5 option-S activate 6.5 mm avg; magnet TBD + fit test | DS + TBD |
| M4 | PCB antenna variant C1 (not U-N4); keepout rule → #5 | DS |

## Pricing roll-up (live evidence, retrieved 2026-09-11, LCSC API snapshot)

Prototype qty 1, USD, API-listed single-piece prices; **not a quote**.

| C# | Price @1 | Price @~proto lot |
|---|---|---|
| C1 | 3.2912 | 2.6925 @30 |
| C2 | 0.1014 | 0.0687 @150 |
| C3 panel | TBD — not in catalog; sold via maker shop, no price retrieved | TBD |
| C4 | 0.1104 | 0.0768 @150 |
| C5 | 0.6589 | 0.4930 @30 |
| C7 | 0.0742 | 0.0598 @200 |
| C8 | 0.1639 | 0.1117 @150 |
| C9 | 0.0162 (UMW C5158048 @1…499 tier 0.0186; @1 listed 0.0162 snapshot) | 0.0143 @500 |
| C10 ×2 | 0.0188 | 0.0121 @2k |
| C11 | 0.0154-class (XH listings) | — |
| C13–C19 | ≤ $0.09 each line @1 | — |
| **PCBA subtotal (schematic parts)** | ≈ **$4.9 @1** | ≈ $3.8 mid-lot |

Panel + magnet + cable + PCB + enclosure remain TBD (no fabricated numbers).
Planning target USD 30–55/unit (hardware/requirements.md) is comfortably
reachable even at 2–3× these figures; no ceiling risk identified.

## Execution re-verification (2026-09-12)

During the 2026-09-12 executor run, the load-bearing citations were
independently re-checked against raw sources (this is document + catalog
verification, **not** bench measurement):

* ESP32-C3-WROOM-02 Table 3-1 pin map and the EN RC-delay guidance
  ("R = 10 kΩ and C = 1 µF") re-extracted from the SHA-256-matched v1.7 PDF;
  the `Espressif:ESP32-C3-WROOM-02` KiCad symbol pin map was re-extracted
  from the `.kicad_sym` and matches datasheet Table 3-1 pin-for-pin.
* ME6217 DS: 800 mA / dropout 100–180 mV @ 300 mA / VIN abs-max 7.0 V /
  PD 1000 mW SOT-89-5 / SOT-89-5 pin map all re-confirmed from PDF text.
  The manifest SHA for `me6217.pdf` was corrected this run (original row
  carried a stale hash; re-download from the same JLC-hosted URL verified).
* AP2112 DS: θJA 184 (SOT25) / 120 (SOT89-5) °C/W re-confirmed — Finding 1
  arithmetic stands.
* USBLC6-2SC6 DS: IEC 61000-4-2 level 4 (15 kV air / 8 kV contact)
  re-confirmed.
* Littelfuse 59170 DS: S/T/U sensitivity table + "57045 Actuator is sold
  separately" note re-confirmed.
* Live catalog spot-check via jlcsearch.tscircuit.com (2026-09-12):
  C81592 (ME6217C33P5G) stock 11 127, C2934560 (ESP32-C3-WROOM-02-N4)
  stock 3 702, C1525 (CL05B104KO5NNNC) stock 16 407 331 — matches the
  2026-09-11 snapshot figures exactly.
* `npx markdownlint-cli2@0.13.0 "**/*.md"` run locally: 0 errors.

## Datasheet manifest update

Added to [`datasheet-sources.md`](datasheet-sources.md) (paths under
`datasheets/` are gitignored binaries; the manifest is committed):
GDEY029T94 official DS, ME6217 DS, AP2112 DS39724 Rev 2, USBLC6 DocID11265
Rev 5, Littelfuse 59170 DS, Nexperia PESD5V0S1Bx (2024), vendor drawings
(USB-C, FPC, button). Baseline rows (Espressif ×2) re-verified: SHA-256
unchanged.

## Verification gaps / open items (explicit)

1. **JLC/LCSC evidence is snapshot-backed for lifecycle fields**
   (`fetched_live: false` on resolve; stock/price via jlcsearch on
   2026-09-11). DigiKey/Mouser live lifecycle + pricing **not retrieved**
   (no API keys) — "Active" status is catalog-attribute evidence only.
2. **ME6217 θJA not published** — thermal adequacy argued from PD = 1 W +
   copper design; validate pour + via pattern in #5, measure at #8.
3. **Panel FPC pitch/contact-side not in the text layer** — must be
   visually verified from the DS drawing against C4 in #5 (M2).
4. **Driver IC identity of GDEY029T94 not stated in its DS** — do not
   claim SSD1680; #6 firmware uses the panel command set (DS §7).
5. **Magnet (C6) and UART header (C12) MPNs TBD** — unguessed by design.
6. **UMW PESD5V0S1BA datasheet** not retrieved from UMW directly (Nexperia
   family DS used) — re-confirm or switch to TECH PUBLIC before fab (#9).
7. Button electricals from catalog attributes only (1-page Chinese drawing).
8. No KiCad schematic exists yet — pin-level *schematic* wiring is #3's
   job; this issue fixes components, footprints chosen in #3 from the same
   datasheet packages (module land pattern: DS Fig. 11-1).
