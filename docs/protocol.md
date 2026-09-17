# Gear Miles — Device/App Protocol

Version: `1` (frozen). The wire field `v` in every JSON response carries this
integer (firmware `API_PROTO_V`) for version negotiation. Breaking changes
bump the version, are recorded in the changelog at the bottom, and clients
that see `v` newer than their supported version must degrade gracefully
(surface raw values, stop sending config writes).

Contract owner: this document + the firmware handlers in
`firmware/components/domain/src/gm_api.c`. The dashboard client types mirror
it (`app/src/types.ts`); contract tests
(`app/tests/contract/api.contract.test.ts`) run the client surface against a
stub that links the real firmware code.

## Transport & scope

- Plain HTTP/1.1 over IPv4 LAN only, served by the device on `:80`
  (station mode) or `:80` on `192.168.4.1` (setup-AP mode).
  mDNS: `gearmiles.local`.
- No TLS, no authentication, no accounts. Threat model: home LAN with no
  personal identifiers in payloads. This is an explicit, documented
  limitation — users needing isolation should put the device on a guest VLAN.
- The device makes **no outbound requests** except optional SNTP (time only).
- The built dashboard bundle contains no external URLs (CI asset audit).

## Endpoints (v1, implemented)

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/status` | Live session + device health JSON (schema below) |
| GET | `/api/sessions?limit=N` | Recent session summaries (limit default 20, max 50) |
| GET | `/api/sessions/<id>` | Single session record |
| GET | `/export.csv` | Full history export (user-initiated) |
| POST | `/api/session/toggle` | Remote session toggle (mirrors BTN_A taps) |
| POST | `/api/config` | Validate-and-set config subset (all-or-nothing) |
| GET | `/api/wipe/nonce` | Fetch short-lived single-use wipe nonce |
| POST | `/api/data/wipe` | Nonce-gated data wipe |
| GET | `/`, `/assets/*` | Embedded static dashboard SPA |

### `/api/status` response

```json
{
  "v": 1,
  "state": "running",
  "session_id": 41,
  "elapsed_s": 1834,
  "distance_km": 21.430,
  "speed_kmh": 42.1,
  "cadence_rpm": 92,
  "estimate": true,
  "estimate_basis": "crank cadence x configured circumference",
  "history_slots_free": 471,
  "dropped_records": 0,
  "wifi_connected": true,
  "display_error": false,
  "uptime_s": 86000
}
```

`state` ∈ `idle | running | paused | finished | settings |
factory_reset_confirm`.

### Sessions list / detail

`GET /api/sessions` → `{"v", "schema_version": 1, "estimate": true,
"estimate_basis", "sessions": [{"id", "started_epoch", "elapsed_s",
"distance_km", "avg_rpm", "max_rpm", "estimate": true}, …]}` newest-first.
`GET /api/sessions/<id>` returns the same record fields plus `v`,
`schema_version`, `estimate_basis`; `404 {"error":"not_found"}` when absent.

### `/export.csv`

`text/csv`; header lines carry the version and estimate legend:

```text
# schema_version=1
# estimate_basis=crank cadence x configured circumference
id,started_epoch,elapsed_s,distance_km*,avg_rpm,max_rpm
```

The `*` on `distance_km*` marks estimates. (JSON export is **not** part of
v1 — see changelog / filed issue.)

### `/api/config` (POST, flat JSON object)

Accepted keys: `circumference_mm` (300–4000), `gear_num`/`gear_den` (1–99,
wheel revs per crank rev), `debounce_ms` (5–100), `ema_percent` (5–100),
`dropout_ms` (100–5000), `units` (0=metric, 1=imperial), `full_every`
(1–250 forced full e-ink refresh interval). `ssid`/`pass` strings are
accepted for the captive flow only. Unknown keys are ignored. Batches are
all-or-nothing: first validation failure returns
`400 {"field":"…","reason":"…"}` with machine-readable `field`/`reason`
(`out_of_range`, `invalid`, `too_long`, `malformed_json`) and nothing is
applied. Success: `200 {"ok":true}` (persisted to NVS).

### Wipe flow

1. `GET /api/wipe/nonce` → `{"nonce":N,"ttl_note":"single-use"}`
2. `POST /api/data/wipe` with `{"nonce":N}` → `200 {"wiped":true}`;
   any other/stale nonce → `403 {"error":"nonce_mismatch"}`.
   A nonce is consumed by the first accepted call.

## Semantics

- All `distance_km` / `speed_kmh` values are **estimates** derived from crank
  cadence × configured effective circumference. Clients must surface the
  `estimate_basis` string or equivalent labeling — firmware enforces
  `estimate: true` sibling fields; the dashboard shows the basis verbatim.
- `session_id` is a monotonic u32 ring sequence; clients key on it. History
  lives in a fixed ring; `history_slots_free` and `dropped_records` expose
  storage state (no silent overwrite).
- Times: `started_epoch` is cosmetic (0 = no SNTP time); `elapsed_s` and
  `uptime_s` are monotonic-device-clock values and authoritative.

## Security assumptions (stated, not assumed silently)

- LAN-only exposure; the device never port-forwards or phones home.
- Wipe requires a nonce fetched immediately before the call (prevents casual
  one-request data destruction on a shared LAN).
- No credentials are transmitted except the Wi-Fi passphrase during captive
  setup (`gearmiles-setup` AP at `192.168.4.1`), over HTTP within the
  setup-only network — documented risk, setup window is minutes.
- Firmware logs redact SSID/passphrase.

## Compatibility

- Clients must ignore unknown response fields; `v` is the negotiation field
  (present on every JSON response).
- `schema_version` versions the CSV/JSON export and sessions-list payloads
  independently of `v`.
- Route paths in v1 are permanent; new endpoints are additive.

## Changelog

### v1 (2026-09-17, frozen with issue #7)

Reconciled against the implemented firmware (`API_PROTO_V` bumped 0 → 1):

- Session control is `POST /api/session/toggle` (state-machine mirror);
  the draft's separate `/api/session/start` + `/api/session/stop` are not
  implemented and are dropped from the contract.
- `/export.json` was drafted but not implemented; v1 ships CSV only
  (gap tracked in a filed issue, not silently patched away).
- Status payload renamed from the draft's `distance_m_est`/`speed_kmh_est`
  to `distance_km`/`speed_kmh` with an explicit `estimate: true` flag +
  `estimate_basis` (one place enforces labeling).
- Added `dropped_records`, `wifi_connected`, `display_error`, `uptime_s`
  (device-health panel fields).
- Nonce fetch is `GET /api/wipe/nonce` (draft only specified a
  confirm-token, not its endpoint).
- Config accepted-keys list is now normative with validation ranges.
