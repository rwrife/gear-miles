# Gear Miles — Firmware

**Status: planning only.** No firmware exists yet; no build or test result is
claimed.

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
