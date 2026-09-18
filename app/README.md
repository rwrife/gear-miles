# Gear Miles — Companion App (device-hosted web dashboard)

**Status: implemented (issue #7).** TypeScript + Vite SPA, built and embedded
into the firmware image as `firmware/main/web_assets.c` (raw bytes, ~16 kB).
No CDN, no analytics, no external fonts, no service worker — the CI asset
audit fails the build if any external host appears in the bundle.

## Layout

```text
app/
  index.html            SPA shell (semantic + ARIA, keyboard-first tabs)
  src/types.ts          protocol v1 types (mirrors docs/protocol.md)
  src/api.ts            same-origin fetch client
  src/format.ts         pure formatting/aggregation helpers
  src/main.ts           view logic: live, history+trend, settings, device
  src/style.css         contrast-checked palette, phone-first
  scripts/embed_bundle.mjs  dist/ -> firmware C table + sha256 manifest
  scripts/audit_assets.mjs  external-host bundle audit (CI gate)
  tests/contract/       API contract tests vs the firmware code itself
  tests/a11y/           Playwright keyboard/contrast/viewport smoke
```

## Form factor decision

The companion is a **responsive web app hosted by the device itself**
(TypeScript + Vite build embedded in firmware flash), not a native mobile
app. Rationale: zero install on phones/desktops, single codebase, works
offline on the LAN, and keeps the whole product local-first.

## Build & verify locally

```bash
npm install
npm run build                          # tsc --noEmit && vite build -> dist/
node scripts/audit_assets.mjs          # bundle has no external hosts
node scripts/embed_bundle.mjs          # regenerate firmware embed + manifest
node scripts/embed_bundle.mjs --check  # CI: committed embed == fresh build
```

### Contract tests (run against real firmware code, not a re-implementation)

`firmware/test/stub` links the actual `gm_api.c` handlers into a tiny HTTP
server. Contract tests are host/simulation evidence only.

```bash
make -C ../firmware/test/stub
GM_STUB_DIST=$PWD/dist ../firmware/test/stub/gm_stub &   # one origin, like the device
npm run test:contract
npm run test:a11y                                        # needs playwright browsers
```

CI (`.github/workflows/app.yml`) runs exactly this sequence, then a firmware
host test (`web_assets_lookup`) proves the committed embed compiles and
resolves every referenced asset path.

## Embedding into the firmware image (integration step with hash check)

The embed is **committed** (`web_assets.c/.h/_manifest.json`) so `idf.py
build` needs no Node toolchain. Freshness is enforced in CI: the app job
rebuilds `dist/` from sources and `embed_bundle.mjs --check` byte-compares
the generated table + sha256 manifest against the committed files. Flash
`firmware/build/gear_miles.bin` and the dashboard is served at `/` +
`/assets/*` on the device. To change the UI: edit `src/`, `npm run build`,
`node scripts/embed_bundle.mjs`, commit both.

## Responsibilities (implemented)

- **Live status**: 1 s polling of `/api/status`, estimate labeling from the
  device's own `estimate_basis` string
- **History**: session table + weekly (Monday-keyed) distance trend
- **Data ownership**: CSV download, wipe via nonce flow (issue nonce →
  confirm → post nonce; device enforces single use)
- **Settings**: config form; rejections surface the device's machine-readable
  `field`/`reason`
- **Device health**: protocol version, uptime, Wi-Fi/display state, free
  history slots, dropped records

## Setup flow note

The first-boot captive portal is firmware-side (`wifi_setup.c`, SSID
`gearmiles-setup` at `192.168.4.1`). The LAN dashboard intentionally does
not carry a Wi-Fi credential form: SSID/pass are accepted by `/api/config`
for the setup context only, and the captive flow owns that UI. After joining
the LAN the dashboard is at `http://gearmiles.local/`.

## Data ownership

- All history lives on-device; the browser is a stateless viewer (no
  localStorage beyond nothing — the app stores no state in the browser)
- Export files are user-initiated downloads; nothing uploads anywhere

## Non-goals

- Accounts, sync between users, social features
- Health dashboards, calorie/medical claims
- Cloud relay or remote access from outside the LAN

## Known gaps (filed as issues, not silently patched)

- `avg_rpm` per session is the last cadence reading (firmware records
  `cad.rpm` at commit); true averaging is issue #8 bench tuning
- Live session per-second sample stream (draft `/api/sessions/<id>` detail
  with samples) is summary-only in v1
