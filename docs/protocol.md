# Gear Miles — Device/App Protocol (initial draft)

Version: `0-draft`. This contract is owned by the firmware and dashboard backlog
issues; breaking changes bump the version and are recorded here.

## Transport & scope

- Plain HTTP/1.1 over IPv4 LAN only, served by the device (`:80` for setup
  mode, `:80` after joining the user network; mDNS `gearmiles.local`).
- No TLS, no authentication, no accounts. Threat model: home LAN with no
  personal identifiers in payloads. This is documented honestly as a limitation;
  users needing protection should isolate the device on a guest VLAN.
- The device makes **no outbound requests** except optional SNTP (time only).

## Endpoints (draft)

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/status` | Live session state JSON (see schema) |
| GET | `/api/sessions?limit=N` | Recent session summaries |
| GET | `/api/sessions/<id>` | Single session detail (per-second samples) |
| GET | `/export.csv` / `/export.json` | Full history export (user-initiated) |
| POST | `/api/session/start` / `stop` | Remote start/stop (mirrors buttons) |
| POST | `/api/config` | Validate-and-set config subset (SSID only via captive flow) |
| POST | `/api/data/wipe` | Confirm-tokened wipe |
| GET | `/` + `/assets/*` | Static dashboard SPA |

### `/api/status` example (illustrative shape, not implemented)

```json
{
  "v": 0,
  "state": "running",
  "session_id": 41,
  "elapsed_s": 1834,
  "distance_m_est": 21430,
  "speed_kmh_est": 42.1,
  "cadence_rpm": 92,
  "estimate_basis": "crank cadence x configured circumference",
  "history_slots_free": 471
}
```

## Semantics

- All "est" fields are estimates derived from cadence × configured circumference;
  clients must surface the `estimate_basis` string or equivalent labeling.
- `session_id` is a monotonic u32 ring sequence; clients key on it.
- Configuration writes are validated server-side; out-of-range values are rejected
  with `400` + machine-readable `field`/`reason`.

## Security assumptions (stated, not assumed silently)

- LAN-only exposure; device never port-forwards or phones home
- Wipe requires a short-lived nonce fetched immediately before the call
  (prevents casual one-request data destruction on a shared LAN)
- No credentials transmitted except the Wi-Fi passphrase during captive setup,
  over HTTP within the setup-only AP — documented risk, setup window is minutes
- Firmware logs redact SSID/passphrase

## Compatibility

- Clients must ignore unknown fields (`v` integer for future negotiation)
- CSV/JSON export schemas are versioned with their own `schema_version` header row/field
