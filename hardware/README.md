# Gear Miles — Hardware

**Status: planning only.** No KiCad project, schematic, PCB, prototype, or
measurement exists yet. This file describes the intended system.

## System block diagram

```text
USB-C 5V ──▶ 5V/3.3V LDO ──▶ ESP32-C3 module (Wi-Fi, flash)
                               │
             ┌─────────────────┼──────────────────┐
             ▼                 ▼                  ▼
      E-ink 2.9" SPI     Reed/Hall input     2× tactile buttons
      (FPC connector)    (ESD + pull-up,      (GPIO w/ interrupts)
                         crimp 2-pin header)
             │
      Antenna keepout zone (module PCB antenna over board edge)
```

## Controller choice

ESP32-C3-based module with a PCB antenna (candidate families: ESP32-C3-WROOM-02
or ESP32-C3-MINI-1 — final MPN selected with datasheet evidence in the
component-selection issue; the power budget in `docs/architecture.md` cites
ESP32-C3-WROOM-02 & WROOM-02U Datasheet v1.7). Rationale: ultra-low cost,
single-core sufficient for 1 Hz UI + cadence
IRQ, native USB for flashing/diagnostics, huge community, and the same toolchain
as the rest of the tool-lab hardware projects.

## Interfaces

| Interface | Direction | Notes |
|---|---|---|
| USB-C 5 V | power + USB CDC flash/debug | 5 V SELV only; no HV anywhere |
| Reed/Hall sensor input | in | Open-drain sensor with pull-up; ESD protection on external connector |
| E-ink 2.9" SPI panel | out | 4-wire SPI + BUSY; FPC connector with documented pinout |
| Buttons ×2 | in | Start/stop, scroll/enter; interrupts with debounce |
| Test points | — | 3V3, 5V, sensor, SPI CLK/MOSI, BOOT, GND |
| Programming header | — | UART/USB boot points plus labeled test pads |

## Power plan

Normative numbers live in `docs/architecture.md` §4 (power budget).
Directional notes:

- Input: USB-C, 5 V SELV; Wi-Fi TX bursts (module datasheet peak 345 mA @
  3.3 V, WROOM-02 DS v1.7 Table 6-4) dominate the peak, not the display
  refresh (~9–13 mA during updates)
- 3.3 V LDO **≥ 500 mA class** (floor set by module TX peak; exact part in
  issue #2) with bulk + MLCC decoupling per module datasheet
- No battery, no charger, no buck/boost, no mains — SELV-only by design
- Deep-sleep policy: display holds last frame; controller idle between ticks;
  measured current documented at bring-up (no fabricated numbers here)

## Enclosure / assembly concept

- 3D-printable clip-on housing near the crank bottom bracket for the sensor head
  (magnet gap spec documented); card-guide style mount for the e-ink panel on the
  handlebar area; printed stand option for the main unit beside the bike
- No metal enclosure material within the antenna keepout
- Fasteners: M2 self-tapping into plastic bosses; no threaded standoffs required

## Safety limits

- USB 5 V SELV only; the device contains no mains, no batteries, no heaters, no
  relays, no motor drive
- Not a medical device; displays no health advice; "distance/speed" are estimates
  derived from crank cadence × user-entered circumference and must be labeled as
  estimates in every UI surface
- Sweat-splash indoor environment only; no outdoor/waterproof claim

## Expected KiCad deliverables (created by backlog issues, not yet present)

- `hardware/kicad/gear-miles.kicad_pro` — project file
- `hardware/kicad/gear-miles.kicad_sch` — complete editable schematic
- `hardware/kicad/gear-miles.kicad_pcb` — two-layer carrier layout
- ERC + DRC reports committed alongside; every exception documented
- MPN/Manufacturer populated as KiCad symbol properties; `bom/bom.csv` exported
  from the schematic as source of truth
- PDF schematic + board renders as *supplements only*
