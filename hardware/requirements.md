# Gear Miles — Requirements (planning baseline)

All targets are design goals to be verified at bring-up with real measurements.
No value here is a measurement.

## Electrical

| ID | Requirement | Verification |
|---|---|---|
| E1 | Powered from USB-C, 5 V ±5%, ≤ 150 mA peak budget (3.3 V rail + display refresh included) | Bench current trace at bring-up |
| E2 | 3.3 V rail within module datasheet tolerance at all loads incl. e-ink refresh peak | Scope capture + ERC/DRC review |
| E3 | Cadence input tolerates open-drain reed/Hall sensors with 10–100 kΩ external pull-up options; input ESD protected (IEC 61000-4-2 contact-level component choice per datasheet, not a claim of passed certification) | Schematic review + bench |
| E4 | Cadence detection 0–200 RPM with ≤ ±2 RPM error vs manual reference count at 40/80/120 RPM | Bench test, 3 runs each |
| E5 | All external connectors ESD-protected; no GPIO drives > datasheet absolute max on any pin | Datasheet review + ERC |
| E6 | No battery, no charging circuit, no mains, no > 24 V node anywhere | Schematic/DRC + visual |

## Mechanical

| ID | Requirement | Verification |
|---|---|---|
| M1 | Main board ≤ ~70 × 50 mm, 2 layers | Layout bounds |
| M2 | E-ink panel connector + display mount accommodate a 2.9" class panel FPC with documented envelope | Datasheet + printed mock |
| M3 | Sensor head clips to frame with magnet gap ≤ sensor datasheet operate distance across ≥ 2 crank styles | Fit test |
| M4 | Antenna keepout free of copper, metal, and enclosure material | DRC + assembly check |

## Environmental

- E-temperature: 0–40 °C indoor; storage −10–50 °C
- Humidity: non-condensing; sweat-splash resistible (conformal coat optional note)
- Not rated for outdoor use or water jets

## Connectivity

- Wi-Fi 2.4 GHz 802.11 b/g/n (module-class); static or DHCP LAN address;
  mDNS `gearmiles.local` best-effort
- First-boot captive-portal setup; USB serial config fallback
- HTTP API on LAN only; no WAN, no TLS promise, no accounts — documented limits
- Offline-first: full standalone operation with zero network config

## Performance

- Speed/distance/cadence update on display ≤ 1 s after each valid cadence second
- E-ink partial refresh completes ≤ panel datasheet max; full refresh every N
  partials to clear ghosting (N tuned at bring-up, documented)
- Session history: ≥ 500 sessions bounded ring in flash; power-loss safe writes

## Cost (planning target, not a quote)

- Prototype BOM target USD 30–55 per unit (excluding host computer, tools,
  shipping, tax), ceiling USD 75
- Prices/availability are TBD until live sourcing validates
  `bom/preliminary-bom.csv` into `bom/bom.csv`

## Data / privacy requirements

- No outbound internet traffic in any steady state (verified by network capture at
  bring-up)
- User data never leaves device except user-initiated export or LAN dashboard view
- Factory reset via button combo wipes NVS + session log
