# Gear Miles — PLAN

## Scope

A single-device, indoor, USB-powered cycle odometer:

- Cadence input from a magnet on the crank arm sensed by a reed or Hall sensor
- Speed/distance derived from cadence × user-configured effective circumference
  (the retrofit honesty note: for a friction/air bike this is a proxy metric and
  must be labelled as such in UI and docs)
- E-ink display with two buttons for stand-alone operation
- Local Wi-Fi for a device-hosted dashboard (setup + history + export); the device
  is fully useful without network
- Bounded local session history with user-controlled retention and deletion

Explicitly out of scope: heart rate, power measurement, medical/fitness advice,
cloud sync, GPS, battery design, mains anything, smart-bus integration.

## Architecture

```
[Crank magnet] → [Reed/Hall sensor] ──GPIO IRQ──▶ ESP32-C3
                                                  │ debounced tick → cadence EMA
                                                  │ speed/dist integration @ 1 Hz
        [E-ink 2.9" SPI panel] ◀──partial-refresh UI driver──┤
        [2× buttons] ────────────────────────────┤
        [Flash NVS + session log ring] ──────────┤
        [Wi-Fi (STA + captive portal first boot)] ┤
                                                  ▼
                              HTTP server on device (LAN only)
                              ├─ /api/status  (live session JSON)
                              ├─ /api/sessions (history JSON)
                              ├─ /export.csv, /export.json
                              └─ static dashboard (Vite build, embedded)
```

## Technology choices (with rationale)

| Choice | Rationale |
|---|---|
| ESP32-C3 (module with PCB antenna + antenna keepout) | Cheap, ubiquitous, Wi-Fi built in, single-core is ample, USB-C native USB for flashing/debug |
| Reed or Hall-effect cadence sensor with mounting magnet, off-the-shelf | Proven bike-computer approach, no optical fouling, SELV |
| 2.9" e-ink SPI panel (no touch) | Readable in bright rooms, low glare at night, no burn-in on static digits, partial refresh fits 1 Hz updates |
| USB-C 5 V power (no battery) | Eliminates charger/BMS safety scope; indoor outlet near bikes is normal |
| Two tactile buttons | Start/stop and scroll are the only stand-alone controls needed; menu kept tiny |
| ESP-IDF firmware | Reproducible headless builds, stable BLE/Wi-Fi stacks; final pick validated in firmware issue |
| Device-hosted TypeScript/Vite web app, embedded in flash | Companion with zero install on user phone/desktop, no external CDN, offline-capable |
| Plain HTTP on LAN only, no TLS/accounts | Honest threat model: home LAN only, no personal identifiers, documented limits. No fake security claims |

## Milestones & dependency order

1. **M0 Docs/requirements** (this scaffold + issue #1) — measurable requirements,
   risk review, metric-honesty policy (proxy-metric labeling).
2. **M1 Component selection** — datasheet-backed parts; Manufacturer/MPN in KiCad
   symbol properties; voltage/current/pinout/lifecycle checks. Depends on M0.
3. **M2 Schematic + ERC** — real `hardware/kicad/gear-miles.kicad_pro` +
   `.kicad_sch`: power input/USB-C, C3 module, display connector, sensor input with
   ESD/protection, buttons, test points, programming header. ERC clean or documented.
   Depends on M1.
4. **M3 BOM export** — `bom/bom.csv` exported from schematic properties incl.
   non-schematic items (enclosure, magnet, cables, fasteners). Depends on M2.
5. **M4 PCB + DRC** — two-layer carrier, antenna keepout enforced, e-ink FPC
   connector, USB-C power. DRC clean or documented. Depends on M2.
6. **M5 Firmware** — cadence IRQ pipeline (debounce, EMA, dropout timeout), unit
   tests on host/simulation for the math; e-ink driver with refresh budget;
   session log ring in flash; buttons/UI state machine; captive-portal setup.
   Depends on M1 (pin map), can start against dev boards during M4.
7. **M6 Dashboard + protocol** — `/api/*` contract in `docs/protocol.md`, Vite
   dashboard embedded, CSV/JSON export, retention controls. Depends on M5 API stub.
8. **M7 Bring-up + assembly docs** — real measured evidence: cadence accuracy vs
   manual count, refresh latency, standby current; assembly steps; troubleshooting.
   Depends on M4+M5+M6 and physical build.
9. **M8 Fabrication/release** — Gerbers, drill, CPL, BOM, PDF schematic, release
   archive, licenses. Depends on M4 (clean DRC) + M7 learnings.

## Testing strategy

- **Firmware math:** host-compiled unit tests (ESP-IDF `linux_host` tests or a
  thin HAL seam) for debouncing, cadence filtering, distance integration, dropout
  timeout, and session ring wrap.
- **Protocol:** contract tests against the HTTP API using recorded fixtures.
- **Dashboard:** build + lint + accessibility smoke (keyboard nav, contrast).
- **Hardware:** ERC and DRC as gate outputs committed to PRs; bench measurements
  (cadence vs reference count at 3 pedal rates, e-ink refresh time, current draw)
  recorded in bring-up docs with instrument evidence. Simulation results are never
  reported as bench results.

## Packaging / distribution

- Hardware: KiCad sources + Gerber/drill/CPL/BOM zip per release tag
- Firmware: release-binaries (bootloader/partition-table/app) + flash script
- App: embedded in firmware image; no separate install
- Docs: assembly + troubleshooting in-repo

## Risks

- E-ink partial-refresh ghosting on a 1 Hz digits display → mitigate with
  region-refresh strategy and periodic full refresh; validate early on a dev kit
- Reed sensor mounting/repeatability across crank types → document gap/alignment
  spec, include self-test mode counting demo rotations
- Proxy-metric honesty (circumference assumption ≠ actual load/bike) → label speed
  and distance as "estimated from crank cadence × configured circumference"
- ESP32-C3 antenna keepout vs enclosure → keepout respected in layout and enclosure
  material note (no metal over antenna)
- Scope creep toward heart-rate/power/training advice → hard non-goals above

## Non-goals

See README non-goals; additionally: no mobile app store presence, no OTA from the
internet (USB re-flash is the documented recovery path; LAN update only if the
backlog later proves it safe and simple).
