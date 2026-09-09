# Gear Miles — Risk Register

Status: live register (created in issue #1, updated whenever a risk changes
likelihood/mitigation). "Verification method" is how we will *actually
demonstrate* the mitigation works — design intent alone never closes a
risk. Severity likelihood/impact scores are qualitative (L/M/H).

| ID | Risk | L | I | Mitigation | Verification method | Status |
|---|---|---|---|---|---|---|
| R1 | E-ink partial-refresh ghosting accumulates on 1 Hz digits, making values misread | H | M | Region partial refresh + forced full refresh every N partials (N default 30, tuned at bring-up); ghosting test pattern in firmware self-test | Bench: 1 h continuous 1 Hz session on real panel; photos at 10-min intervals reviewed for misread digits (#8) | Open |
| R2 | Reed/Hall sensor mounting variance across crank styles — missed or double counts at gap extremes | H | H | Datasheet operate/release gap drives mount spec; two-position magnet cradle; self-test demo-rotation counter shows tick fidelity before riding; ±2 RPM error gate vs manual count at 40/80/120 RPM | Bench: E4 test protocol, 3 runs × 3 rates × ≥ 2 crank styles, recorded counts vs reference (#8) | Open |
| R3 | Antenna keepout violated by enclosure material/copper → degraded or unusable Wi-Fi | M | H | Keepout exists as an explicit KiCad board rule (rule area, not a comment) from #5; enclosure design forbids metal within the zone; module antenna at board edge | DRC shows keepout violation if copper placed (#5); assembly checklist item; RSSI/packet-loss comparison open-frame vs enclosure at 3 m (#8) | Open |
| R4 | Proxy metric (cadence × circumference) misinterpreted as real speed/distance or training truth | M | H | Normative metric-honesty policy (architecture §7) in every UI surface, API (`"estimate": true`), and export; no calories/health claims | Docs lint/contract test asserts qualifier present on every speed/distance render; manual review of e-ink mockups (#6/#7) | Open |
| R5 | Flash wear from session ring writes bricks or corrupts history | M | M | Append-only ring, batched writes (per-second sample quantized, not per-tick), ≥ 500-session bound, CRC per record, monotonic sequence; wear spread across ring slots | Host unit tests: ring wrap × 10⁴ iterations, torn-tail simulation, CRC reject path (#6); datasheet endurance of chosen flash (module family) vs write budget arithmetic in #6 | Open |
| R6 | LAN-only exposure is mistaken for security; neighbor/rogue device reads or wipes sessions | M | M | Honest threat model in README/protocol (no TLS, no accounts, guest-VLAN advice); confirm-tokened wipe; no outbound traffic in any steady state | Contract tests: wipe requires token; protocol contract test asserts no outbound calls beyond optional SNTP; network capture at bring-up shows zero egress (#7/#8) | Open |
| R7 | 3.3 V brownout during Wi-Fi TX peak if LDO/decoupling undersized → random reboots mid-session | M | H | Power budget (architecture §4) sets ≥ 500 mA LDO class floor; bulk + MLCC decoupling per module datasheet; EN RC per Espressif reference family | ERC/DRC review of power nets (#3); bench scope capture of +3V3 during Wi-Fi TX burst concurrent with refresh (#8) | Open |
| R8 | Panel family EOL / single-source e-ink panel → build stops at sourcing | M | M | Component selection (#2) evaluates second-source panel candidates now, not after layout; FPC + wiring kept to panel-family-common signal set where possible | #2 documents ≥ 2 panel candidates with pinout/pitch cross-check; drop-in feasibility re-checked before fab (#9) | Open |
| R9 | USB-C cable/host cannot source inrush/peak → host-side disconnect, flaky "broken device" reports | L | L | 5.1 kΩ Rd CC straps (proper UFP advertisement, ≤ 500 mA per USB-C), bulk cap limits inrush; troubleshooting doc names cable/host as first suspect | Bench: current trace at plug-in + TX burst on 3 hosts (#8); troubleshooting page updated | Open |
| R10 | Scope creep toward heart-rate/power/training advice (product and code) | M | M | Hard non-goals list in README; risk register owns the "no" — new metric types need a registered issue + this register updated | Maintainer review gate on issues/PRs; README non-goals diffed in any PR touching them | Open (standing) |

## Review cadence

Each executor run that touches hardware/firmware/docs updates Status and
adds rows if new failure modes appear. A risk moves to **Mitigated** only
with a linked evidence artifact (test, measurement record, or committed
report) — not with a claim.
