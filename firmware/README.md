# Gear Miles — Firmware

**Status: domain logic + full application skeleton implemented (issue #6).**
Domain logic (cadence pipeline, odometer math, session ring store, config
validation, session state machine, API handlers, e-ink driver) lives in
`components/` as allocation-free, hardware-independent C with injected
clocks/IO. The ESP-IDF application in `main/` wires it to the frozen GPIO
map. Host unit tests (`test/host/`) run in CI with ASan+UBSan and
`-Werror`; the ESP32-C3 image builds reproducibly in CI (ESP-IDF v5.5.2,
`espressif/idf:v5.5.2`). **No run against real silicon has happened yet** —
all evidence here is host/simulation and build evidence, never bench
evidence. Bench validation is issue #8.

## Layout

```text
components/domain/     cadence, odometer, ring store, config, SM, API (pure C)
components/panel_hal/  GDEY029T94 driver behind an injected bus (pure C)
main/                  ESP-IDF app: board pins, SPI bus, buttons/ISR, NVS,
                       raw-partition ring, UI renderer, LAN HTTP server,
                       captive portal
test/host/             host unit tests (make -C firmware/test/host test)
```

## Build + flash (ESP32-C3)

Requirements: ESP-IDF **v5.5.2** (pinned; CI uses `espressif/idf:v5.5.2`).

```bash
cd firmware
idf.py set-target esp32c3
idf.py build
idf.py -p /dev/ttyUSB0 flash    # native USB CDC of the module
```

Recovery / boot-mode steps:

1. **Normal flash:** hold **BOOT** (GPIO9), press **EN** (reset), release
   EN — module enters ROM download mode over USB CDC; run `idf.py flash`,
   then press EN again.
2. **If USB is dead:** 4-pin UART0 header (3V3/U0_TXD/U0_RXD/GND,
   `docs/architecture.md` §2) into any 3.3 V UART adapter, same BOOT+EN
   sequence, then `idf.py -p /dev/ttyACM0 flash`.
3. **Brick-resilience:** ROM bootloader is always reachable (BOOT/EN on
   the board); there is no firmware path that can remove it (no OTA, no
   GPIO-strapping of the boot ROM).

## E-ink refresh policy (GDEY029T94)

Per panel datasheet (Rev 1.0 §6.3/§7 update timings: full 3 s, fast 1.5 s,
partial 0.3 s): partial refreshes are capped at 1 Hz by the driver
(`PANEL_MIN_PARTIAL_INTERVAL_MS`), and a full refresh is forced every
`full_every` partials (default 30, architecture §6 ghosting row) to clear
accumulated ghosting. BUSY handling polls with a 5 s ceiling; three
consecutive timeouts surface `display_error` through the API (architecture
§6). The driver command sequence is transcribed from datasheet §14.1 and
is **unverified against silicon** until issue #8.

## Responsibilities

1. **Cadence pipeline** — GPIO interrupt on the sensor edge; debounce (configurable
   window); rolling cadence estimate (EMA) with dropout timeout (coasting stops
   counting after a configurable silence); tick rate 0–200 RPM per requirements.
2. **Odometer math** — distance += cadence/60 × crank_revs_to_wheel × configured
   effective circumference per integrated second; speed = smoothed cadence-derived
   value. All outputs labeled as estimates in the UI.
3. **E-ink UI** — partial refresh of value regions ≤ 1 Hz, scheduled full refresh
   to clear ghosting, night-dim mode (no backlight exists; layout choice only),
   session states: idle / running / paused / finished.
4. **Buttons** — start/stop, scroll/enter; long-press combos for reset/settings;
   power-loss-safe state persistence.
5. **Session store** — append-only ring in flash with monotonic sequence numbers,
   CRC per record, bounded retention, factory-reset wipe.
6. **Setup** — first-boot captive portal (Wi-Fi SSID/pass, circumference, units);
   USB serial fallback CLI; credentials stored in NVS only.
7. **Networking** — LAN HTTP server exposing the documented JSON/CSV API
   (see `docs/protocol.md`); mDNS advertisement; zero outbound internet calls.
8. **Timekeeping** — best-effort SNTP when network exists; monotonic elapsed-time
   timing is authoritative for a session (offline-safe by design).

## Interfaces / protocols

- SPI + BUSY GPIO for the e-ink panel (driver selected per panel datasheet)
- GPIO interrupt for cadence input; internal + external pull options
- USB-CDC console for logs (verbosable, no secrets in logs)
- HTTP/JSON API per `docs/protocol.md`

## Provisioning / update approach

- Flash + recovery via USB (esptool / `idf.py flash`); documented boot-mode steps
- No internet OTA in MVP. LAN-side update considered only if the backlog later
  demonstrates a safe signed flow; until then USB re-flash is the only path
- Config changes: buttons on-device or the LAN dashboard; validated ranges, no
  shell-accessible config endpoints

## Test strategy

- **Host/simulation unit tests** (no hardware required): debouncing windows,
  cadence EMA convergence, dropout timeout, distance integration against
  hand-computed tables, ring wrap + CRC recovery, config validation
- **Protocol contract tests** against a simulated firmware stub
- **Static checks**: `clang-tidy`/CI build warnings-as-errors once skeleton lands
- **Bench tests (hardware, later milestone)**: cadence accuracy at 40/80/120 RPM
  vs manual reference count, refresh latency, current draw, power-loss integrity
- Simulation/host-test results are reported as such — never as bench evidence
