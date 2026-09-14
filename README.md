# Gear Miles

**One-sentence pitch:** A local-first, USB-powered ESP32-C3 retrofit odometer for stationary exercise bikes that measures cadence and shows speed, distance, and time on a low-glare e-ink display, with a private browser dashboard — no accounts, no cloud, no subscription.

## Motivation

Most people with a stationary bike, spin bike, or trainer either own a console that
requires batteries and offers no history, or are pushed toward subscription apps that
want an account, a phone, and often a membership fee. Makers who want a simple "how far
did I pedal this week" readout have no open, buildable, local-first option. Gear Miles
fills the gap between a dumb bike computer and a subscription platform: a small PCB you
can design in KiCad, assemble at home, and trust to keep your workout data on your own
network.

## Target users

- Home cyclists and fitness hobbyists with a stationary/spin bike or roller trainer
- Makers who want a realistic, low-cost ESP32 + KiCad open-hardware build
- Privacy-conscious users who refuse account-based fitness platforms

## Concrete use cases

- Clip the sensor on the crank arm, tap Start on the device, pedal; read speed,
  distance, calories-free cadence, and elapsed time on the e-ink display
- Review today's and this week's sessions on a local web dashboard from a phone or
  desktop browser over Wi-Fi — nothing leaves the LAN
- Export session history as CSV/JSON for your own spreadsheet or notes
- Keep working offline: the device logs sessions to flash even with no network

## How to use (intended end-to-end workflow)

1. Assemble the device (carrier PCB + reed/Hall cadence sensor + e-ink display) per
   `hardware/README.md` and `docs/` bring-up notes.
2. Power over USB-C (5 V SELV). On first boot the device opens a setup hotspot;
   join it from a phone/laptop to pick Wi-Fi and wheel/crank circumference.
3. Mount the magnet on the crank arm and the sensor within its gap spec.
4. Press Start. The display shows speed, distance, elapsed time, and cadence,
   refreshed on the e-ink panel (partial refresh; no flicker loops).
5. Press Stop to close the session. Sessions are stored on-device with bounded
   history and synced opportunistically to the device-hosted dashboard.
6. Open `http://gearmiles.local/` (or the device IP) for charts, per-session detail,
   and CSV/JSON export.

## MVP feature list

- Cadence measurement from a reed or Hall-effect sensor on the crank arm
- Configurable wheel/crank effective circumference for speed and distance
- 2.9" class e-ink display: speed, distance, elapsed time, cadence, session state
- Two physical buttons: start/stop session, and scroll/settings
- On-device bounded session history (flash), survives power loss
- Device-hosted local web dashboard: current status, session list, simple trends,
  CSV/JSON export, retention controls, deletion
- First-boot captive-portal Wi-Fi setup; USB fallback configuration
- Non-color-only status indication; readable in a bright room via e-ink

## Non-goals (MVP)

- No heart-rate, power-meter, or medical/health claims (no diagnosis, no training
  prescriptions, no "calories burned" as medical guidance)
- No cloud sync, accounts, or third-party platform integration
- No controlling or sensing smart bikes via their proprietary buses
- No Strava/Komoot integration in MVP (export instead)
- No battery/BMS design — USB 5 V SELV powered only
- No outdoor GPS functionality
- No waterproof/outdoor enclosure claim (indoor/sweat-splash environment)

## Privacy, permissions, and data-storage behavior

- All workout data stays on the device and, optionally, in the browser session that
  viewed it; the device performs no outbound telemetry
- Wi-Fi credentials are the only "secret" stored; they are used solely for LAN access
- The dashboard has no accounts; bind to LAN only, and document this limit
- CSV/JSON export is user-initiated; retention window is user-configurable
- Clearing data or factory reset is a physical button combination on-device

## Current status

**Hardware schematic is NOT FOR FABRICATION; hardware remains unbuilt.** The
repository contains the accepted architecture baseline
([`docs/architecture.md`](docs/architecture.md)), component evidence, and an
editable KiCad 9 source at
[`hardware/kicad/gear-miles.kicad_sch`](hardware/kicad/gear-miles.kicad_sch).
The e-ink pump candidate was rejected in electrical review despite zero ERC.
Diode polarity, full panel-circuit tracing, connector evidence and footprints
remain explicit blockers in [`docs/schematic.md`](docs/schematic.md).
Issue #3 remains incomplete. There is no PCB, firmware build, prototype, or
bench/field measurement, and none is claimed.

### Metric-honesty policy

All speed and distance readouts on any surface (display, dashboard, API,
exports) are **estimates derived from crank cadence × a user-configured
effective circumference** — never measured wheel speed. Every such value
carries an explicit "estimated" qualifier; the normative rules live in
[`docs/architecture.md` § Metric-honesty policy](docs/architecture.md).

Milestones:

1. Requirements + architecture accepted (issue backlog #1)
2. Component selection with manufacturer evidence; KiCad schematic + ERC
3. PCB layout + DRC; BOM exported from schematic properties to `bom/bom.csv`
4. Firmware: cadence pipeline, e-ink UI, local logging — with simulation tests
5. Device-hosted dashboard + protocol
6. Bring-up + assembly docs with real measured evidence
7. Fabrication outputs + release

> Final BOM data belongs in KiCad schematic symbol properties (Manufacturer, MPN,
> etc.) and is exported to `bom/bom.csv`. `bom/preliminary-bom.csv` is a
> planning-only aid and never a source of truth. Hardware sources are editable
> KiCad files (`hardware/kicad/gear-miles.kicad_pro`, `.kicad_sch`, `.kicad_pcb`);
> diagrams/PDFs supplement but never replace them.

## Development / build quickstart (planned, not yet functional)

- **Hardware:** KiCad 8+ project at `hardware/kicad/`. ERC/DRC run headless via
  `kicad-cli`.
- **Firmware:** ESP-IDF (or Arduino-for-ESP32 if the backlog so decides) at
  `firmware/`; `idf.py build`/flash instructions will live in `firmware/README.md`.
- **App:** the companion is a device-hosted static/responsive web app (TypeScript +
  Vite) served by the ESP32; build with `npm run build`, no external services.

## License

MIT (see `LICENSE`).
