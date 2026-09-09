# Gear Miles — Companion App (device-hosted web dashboard)

**Status: planning only.** No app code or build exists yet.

## Form factor decision

The companion is a **responsive web app hosted by the device itself**
(TypeScript + Vite build embedded in firmware flash), not a native mobile app.
Rationale: zero install on phones/desktops, single codebase, works offline on the
LAN, and keeps the whole product local-first. A native app would add store
friction without adding capability for this MVP.

## Responsibilities

- **Setup flow**: on first boot, guide Wi-Fi + circumference + units selection
  (the device's captive portal serves the same UI)
- **Live status**: running-session view (speed/distance/time/cadence, refresh ≤ 1 s,
  with an explicit "estimated from crank cadence" label)
- **History**: session list, simple weekly distance trend, per-session detail
- **Data ownership**: CSV/JSON export, retention window control, wipe-with-confirm
- **Device health**: firmware version, uptime, free session slots, last sync time

## Setup flow (intended)

1. Device first boot → joins captive-portal `GearMiles-SETUP`
2. Browser auto-prompt (or manual `http://192.168.4.1`) → same dashboard SPA
3. Pick Wi-Fi, enter network key, set effective circumference (with a helper:
   "wheel size chart or measure by chalk-and-tape")
4. Device joins LAN; dashboard reachable at `http://gearmiles.local/` thereafter

## Data ownership

- All history lives on-device; the browser is a stateless viewer (no localStorage
  caches beyond UI prefs)
- Export files are user-initiated downloads; nothing uploads anywhere
- Retention + deletion controls mirror firmware settings

## Protocol boundary

The app talks **only** to the device's documented HTTP API
(`docs/protocol.md`). No CDNs, no analytics, no external fonts, no service-worker
update channel. All assets ship in the firmware image so the dashboard also works
with Wi-Fi disabled via USB-serial-off (LAN) — offline-first by construction.

## Non-goals

- Accounts, sync between users, social features
- Health dashboards, calorie/medical claims
- Cloud relay or remote access from outside the LAN
