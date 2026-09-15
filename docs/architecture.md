# Gear Miles — Architecture (v1 baseline)

Status: **accepted v1 baseline** (issue #1). This document freezes the
measurable system baseline: interfaces, GPIO map draft, power budget, session
state machine, error/dropout behavior, and the metric-honesty policy. It is
the reference that issues #2–#9 implement against; changes to frozen
interfaces here require a PR that updates this file and references the
motivating issue.

Every electrical figure below is either (a) quoted from a manufacturer
datasheet with an exact citation, or (b) marked TBD. Every cited document
is recorded in [`datasheet-sources.md`](datasheet-sources.md) with URL,
version, retrieval date, and SHA-256. No value here is a bench
measurement — none exist yet. When bring-up (issue #8) produces real
measurements, this document gains a "measured" column citing instrument
evidence; until then the datasheet values are *design inputs*, not claims
about built hardware.

## 1. Block diagram

```text
USB-C 5V ──▶ 3.3 V LDO ───────────────────────────────────────────────┐
 (5 V SELV)   (class TBD in #2, ≥500 mA)                              ▼
   │  ├─▶ VBUS→GND CC1/CC2 5.1 kΩ UFP straps                  ESP32-C3-WROOM-02
   │  └─▶ D+/D− to module USB (GPIO18/19)                     ┌─────────────────┐
   │    + ESD array on D+/D−                                  │ Wi-Fi 2.4 GHz   │
   │                                                          │ flash, native   │
   │  Antenna keepout (explicit board rule, #5):              │ USB CDC)        │
   │  no copper / no metal enclosure over module PCB          └─┬────┬────┬────┘
   │  antenna zone                                              │    │    │
                                        24-pin FPC  INK_SPI ◀───┘    │    └──▶ CADENCE
                              GDEY029T94 2.9" panel (driver unnamed  │          reed/Hall open-drain
                              driver, on-panel booster caps)         │          input, 10–100 kΩ
                                                                     │          pull-up + ESD
                                              BTN_A (GPIO0), BTN_B (GPIO8)
                                              BOOT TP (GPIO9), U0TXD/RXD header,
                                              test points: 5V, 3V3, CADENCE, INK_CLK,
                                              INK_MOSI, BOOT, GND
```

## 2. Interfaces

| Interface | Direction | Protocol / electrical | Frozen contract |
|---|---|---|---|
| USB-C power | in | 5 V ±5 %, USB 2.0 FS | UFP: 5.1 kΩ Rd on CC1 & CC2, no PD negotiation; D+/D− wired to module USB pins; ESD device selected in #2 |
| USB native (module GPIO18/19) | in/out | USB CDC + Serial/JTAG | Flashing/debug/recovery path; documented recovery when LAN lost |
| UART0 (GPIO20 RX / GPIO21 TX) | in/out | 3.3 V logic, 115200 default | Programming/log header on board; ROM bootloader usable via USB CDC or UART0 |
| E-ink panel | out | 4-wire SPI + DC + RST + BUSY, 3.3 V logic | GDEY029T94 24-pin FPC per datasheet §5; the connector MPN and footprint are TBD until a matching manufacturer drawing and panel-tail overlay are obtained. The booster transcription remains a fabrication blocker; see `schematic.md`. |
| Cadence sensor | in | Open-drain to GND, internal pull config n/a | External pull-up 10–100 kΩ populated value per #2; IEC 61000-4-2-rated ESD device on the 2-pin sensor header line; header keyed/shrouded |
| Buttons ×2 | in | GPIO to GND, firmware debounce | BTN_A = start/stop, BTN_B = scroll/enter; GPIO-interrupt capable |
| BOOT | in | GPIO9 strapping, to GND via button/TP | 10 kΩ pull-up (plus internal weak PU per datasheet); test point + optional button |
| Test points | — | — | 5V, 3V3, CADENCE, INK_CLK, INK_MOSI, BOOT, U0TXD, GND |

Net naming convention (frozen, enforced from #3): `USB_5V`, `+3V3`,
`CADENCE`, `INK_CLK`, `INK_MOSI`, `INK_CS`, `INK_DC`, `INK_RST`, `INK_BUSY`,
`BTN_A`, `BTN_B`, `BOOT`, `U0_TXD`, `U0_RXD`, `USB_D+`, `USB_D-`, `GND`.

## 3. GPIO map draft

Source: ESP32-C3-WROOM-02 & WROOM-02U Datasheet v1.7, Table 3-1 (pin
definitions, pp. 10–11) and ESP32-C3 Series Datasheet v2.4, Table 3-1
(strapping pins, p. 30). The module exposes 15 GPIOs.

| Signal | GPIO | Module pin | Notes |
|---|---|---|---|
| INK_CLK | GPIO6 | 5 | FSPICLK; JTAG MTCK alt unused (debug is USB Serial/JTAG) |
| INK_MOSI | GPIO7 | 6 | FSPID |
| INK_CS | GPIO10 | 10 | FSPICS0 |
| INK_DC | GPIO3 | 15 | |
| INK_RST | GPIO4 | 3 | |
| INK_BUSY | GPIO5 | 4 | |
| CADENCE | GPIO1 | 17 | Also ADC1_CH1 — reserved as digital input only |
| BTN_A | GPIO0 | 18 | Not a strapping pin on ESP32-C3 |
| BTN_B | GPIO8 | 7 | Strapping. Button must be released during EN reset. ESP32-C3 DS v2.4 Table 3-3 makes download mode GPIO9 low **and GPIO8 high**; GPIO8 low with GPIO9 low is not a valid download mode. |
| BOOT | GPIO9 | 8 | Strapping: internal weak pull-up; external 10 kΩ PU to +3V3 + button/TP to GND = download boot when pressed at reset |
| U0_RXD | GPIO20 | 11 | UART0 log/flash header |
| U0_TXD | GPIO21 | 12 | UART0 log/flash header |
| USB_D- | GPIO18 | 13 | USB-C receptacle |
| USB_D+ | GPIO19 | 14 | USB-C receptacle |
| *(reserved)* | GPIO2 | 16 | Strapping; external 10 kΩ pull-up to +3V3. Never tie a GPIO directly to the rail. |
| EN | — | 2 | Pull up to +3V3 with RC per Espressif module reference schematic family (detailed values in #3); never float (datasheet: "Do not leave the EN pin floating") |

Design rules for the map: no strapping pin (GPIO2/8/9) drives external
loads that could hold it in a non-default state at reset; GPIO2 and GPIO8 are
externally pulled high (10 kΩ); all pins above stay in their default 3.3 V I/O domain —
no pin drives beyond the recommended operating conditions of Table 5-2,
ESP32-C3 Series Datasheet v2.4 (requirement E5).

## 4. Power budget

Design inputs only — real numbers come from the issue #8 bench.

3.3 V rail loads:

| Load / mode | Value | Kind | Source |
|---|---|---|---|
| ESP32-C3-WROOM-02, Wi-Fi TX peak (802.11b 1 Mbps @ 20.5 dBm, 100 % duty) | 345 mA | peak | WROOM-02 DS v1.7, Table 6-4, p. 23 |
| Wi-Fi TX 802.11g 54 Mbps | 285 mA | peak | Table 6-4, p. 23 |
| Wi-Fi RX (HT20) | 82 mA | typ | Table 6-4, p. 23 |
| Modem-sleep, CPU idle, 80 MHz, periph clocks off | 13 mA | typ | Table 6-5, p. 23 |
| Light-sleep | 130 µA | typ | Table 6-6, p. 24 |
| Deep-sleep | 5 µA | typ | Table 6-6, p. 24 |
| GDEY029T94 during update | 3.0 mA typ @ 3.0 V | typ | Good Display GDEY029T94 DS Rev. 1.0 §6.2 |
| GDEY029T94 deep sleep | 1–5 µA | typ | Good Display GDEY029T94 DS Rev. 1.0 §6.2 |
| CADENCE pull-up 100 kΩ | ≈ 33 µA | typ | Ohm's law at 3.3 V |
| LDO quiescent current | TBD (per #2 part) | — | datasheet, #2 |

Aggregates (design intent):

| Scenario | 3.3 V rail estimate |
|---|---|
| Steady "connected idle" (modem-sleep, UI frame held, Wi-Fi associated) | ~13–22 mA typ + µA parasitics |
| Display refresh concurrent with RX | ~22–35 mA typ |
| Wi-Fi TX burst (dominant peak) | up to ~350–360 mA, brief at typical duty cycles (345 mA figure is 100 % duty rated) |

Rail and inlet design consequences:

- The LDO must supply the module peak with margin: this **confirms the
  preliminary BOM's "300–500 mA class" row and sets the floor at 500 mA
  candidate parts** in #2, not the display refresh current.
- An LDO draws input current ≈ output current, so the worst-case 5 V
  inlet peak mirrors the 3.3 V peak (~350 mA + margin). Bulk
  capacitance on +3V3 (module datasheet power-up guidance) limits slew.

**Requirement E1 correction (recorded in `hardware/requirements.md`):** the
original E1 text "≤ 150 mA peak budget (3.3 V rail + display refresh
included)" is unsatisfiable at face value — the module family datasheet
alone rates 345 mA TX peak, 2.3× the stated cap. E1 is revised to split
average vs peak instead of silently assuming away the datasheet:

> E1 (rev A): Powered from USB-C 5 V ±5 %. Average input current in
> connected-idle steady state ≤ 150 mA. Peak capability ≥ 500 mA at 3.3 V
> (LDO class) with 5 V inlet able to source ≥ 450 mA transients without
> host disconnect. Both verified at bring-up with a current trace.

## 5. Session and button state machine

Firmware-side behavior (implemented/tested in issue #6; host unit tests
cover every transition below):

States: `IDLE`, `RUNNING`, `PAUSED`, `SETTINGS`, `FACTORY_RESET_CONFIRM`.
Events: `tapA`, `tapB` (button taps, debounced), `longB` (≥ 3 s on BTN_B),
`longAB` (A+B held ≥ 5 s). On the confirm prompt, `tapA` means confirm and
`tapB` means abort.

| State | Event | Next state | Action |
|---|---|---|---|
| IDLE | tapA | RUNNING | start session (seq no., circumference snapshot) |
| IDLE | longB | SETTINGS | enter settings (circumference → units → Wi-Fi pages) |
| IDLE | longAB | FACTORY_RESET_CONFIRM | prompt; requires explicit tapA |
| FACTORY_RESET_CONFIRM | tapA | IDLE | wipe NVS + session ring, then reboot UI |
| FACTORY_RESET_CONFIRM | tapB | IDLE | abort, nothing wiped |
| RUNNING | tapA | FINISHED | stop session |
| RUNNING | tapB | PAUSED | freeze integration; timer paused |
| RUNNING | dropout (5 min, §6) | PAUSED | auto-pause, "sensor?" hint |
| PAUSED | tapA | RUNNING | resume |
| PAUSED | tapB | FINISHED | close session |
| FINISHED | — (after 2 s) | IDLE | commit CRC'd record to flash ring |
| SETTINGS | longB or idle timeout 60 s | IDLE | persist changed settings atomically |
| SETTINGS | tapB | SETTINGS | next page |
| SETTINGS | tapA | SETTINGS | select/adjust current page value |

Session states on display: `idle`, `running`, `paused`, `finished`
(matches firmware README). SETTINGS and FACTORY_RESET_CONFIRM are
transient UI states, never stored session states.

## 6. Error and dropout behavior

| Condition | Behavior | Recovery |
|---|---|---|
| Cadence dropout (no tick for T_drop; default 0.75 s ≈ 1.5 crank revs at 40 RPM — chosen so the ≤ 1 s display-update requirement holds at the lowest rated 40 RPM) | cadence/speed read 0 immediately; elapsed timer keeps running; after 5 min of continuous dropout auto-enter PAUSED to bound flash wear and show "paused – sensor?" | first new tick returns to RUNNING |
| Debounce bounce (< configured window, default 30 ms) | ignored, never counted | n/a |
| BUSY line stuck high beyond panel max refresh budget (datasheet per #2) × 3 retries | driver reports UI error region on display, schedules hard RST pulse + full refresh | self-heals or shows error banner; API exposes `display_error` flag |
| Session ring record CRC failure on read | skip record, increment `dropped_records` counter (visible via `/api/status`) | user-visible honesty; no silent overwrite |
| Wi-Fi loss while running | session logging + display unaffected (offline-first); dashboard sees last state after reconnect; mDNS best-effort | automatic; SNTP timestamp only cosmetic, monotonic clock authoritative for elapsed time |
| Power loss mid-session | ring header + record CRCs make the torn tail detectable on next boot; completed sessions always durable | session ring scan at boot discards torn tail only |
| e-ink ghosting beyond threshold | forced full refresh every N partials (N tuned at #8, default 30) | documented in refresh budget |
| Watchdog / firmware fault | ROM bootloader + USB CDC always available (no bricking path) | USB re-flash documented |

## 7. Metric-honesty policy

*(Normative; referenced from README. Any UI surface, API field, or export
column showing speed or distance MUST obey this.)*

1. All speed and distance values are **estimates derived from crank
   cadence × the user-configured effective circumference**. They are not
   measured wheel speed, power, or calibrated distance.
2. Every display of a speed or distance value — e-ink UI, dashboard
   live view, history, CSV/JSON export — carries the qualifier
   "estimated from crank cadence × configured circumference" (or the
   shorter `*estimated*` asterisk + legend in compact UI surfaces).
3. The setup flow explains circumference selection in plain language
   (measure chalk-and-tape, or pick a listed wheel size).
4. No calories, no medical/fitness interpretation anywhere, consistent
   with README non-goals.
5. API fields `speed_kmh` and `distance_km` include sibling field
   `"estimate": true` (protocol v1, issue #7), so consumers cannot
   accidentally treat them as measured.

## 8. Open questions (deliberately not assumed)

| # | Question | Owner / decision point |
|---|---|---|
| Q1 | Exact e-ink panel MPN vs alternatives (panel family is EOL-flagged at Good Display; second-source panel candidates) — includes FPC pitch/count confirmation | issue #2 with distributor + datasheet evidence |
| Q2 | Reed vs Hall for the cadence sensor (gap tolerance vs power draw) | issue #2 |
| Q3 | Final LDO part: must be ≥ 500 mA, ≤ 500 mV dropout at 500 mA, and fit SOT-223/SOT-23-5 with thermal check | issue #2, thermal-checked |
| Q4 | Whether GPIO1 needs the ADC2-caution workaround for coexistence (ESP32 ADC2 vs Wi-Fi) — CADENCE is digital-only here but revisit if battery voltage sensing is ever added | issue #2, revisit before #6 |
| Q5 | Enclosure wall material/dielectric over the antenna zone | bring-up #8 with real part |
| Q6 | Default T_drop 0.75 s and full-refresh interval N=30 are design guesses pending bench cadence profiles | issue #8 tunes and updates this file |
