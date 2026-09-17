import { api } from "./api";
import { fmtElapsed, fmtEpoch, weeklyTotals } from "./format";
import type { StatusResponse, SessionSummary } from "./types";

const $ = <T extends HTMLElement>(id: string): T =>
  document.getElementById(id) as T;

const tabs = ["live", "history", "settings", "health"] as const;
type Tab = (typeof tabs)[number];

let connected = false;
let pollTimer: number | undefined;

function showTab(tab: Tab): void {
  for (const t of tabs) {
    const btn = $(`tab-${t}`);
    const view = $(`view-${t}`);
    const sel = t === tab;
    btn.setAttribute("aria-selected", String(sel));
    btn.classList.toggle("active", sel);
    view.hidden = !sel;
  }
  $(`view-${tab}`).focus();
}

function setConnected(ok: boolean): void {
  if (ok === connected) return;
  connected = ok;
  const el = $("conn-status");
  el.textContent = ok ? "Connected to device" : "Device unreachable — retrying…";
  el.classList.toggle("offline", !ok);
}

function renderStatus(s: StatusResponse): void {
  $("m-state").textContent = s.state;
  $("m-elapsed").textContent = fmtElapsed(s.elapsed_s);
  $("m-distance").textContent = s.distance_km.toFixed(2) + " km";
  $("m-speed").textContent = s.speed_kmh.toFixed(1) + " km/h";
  $("m-cadence").textContent = `${s.cadence_rpm} rpm`;
  /* Metric-honesty policy: surface firmware's own basis string when present,
   * fall back to the static label. Never show est metrics unlabeled. */
  const note = $("live-estimate-note");
  note.textContent = s.estimate
    ? `* Estimated — ${s.estimate_basis}. Not a measured speed or distance.`
    : "* Measured values.";

  $("h-proto").textContent = String(s.v);
  $("h-uptime").textContent = fmtElapsed(s.uptime_s);
  $("h-wifi").textContent = s.wifi_connected ? "yes" : "no";
  $("h-display").textContent = s.display_error ? "ERROR" : "ok";
  $("h-slots").textContent = String(s.history_slots_free);
  $("h-dropped").textContent = String(s.dropped_records);
}

function renderSessions(rows: SessionSummary[]): void {
  const body = $("sessions-body");
  body.textContent = "";
  $("sessions-empty").hidden = rows.length > 0;
  for (const r of rows) {
    const tr = document.createElement("tr");
    const cells = [
      String(r.id),
      fmtEpoch(r.started_epoch),
      fmtElapsed(r.elapsed_s),
      r.distance_km.toFixed(2) + " km",
      String(r.avg_rpm),
      String(r.max_rpm),
    ];
    for (const c of cells) {
      const td = document.createElement("td");
      td.textContent = c;
      tr.appendChild(td);
    }
    body.appendChild(tr);
  }
}

function renderWeekly(rows: SessionSummary[]): void {
  const chart = $("weekly-chart");
  chart.textContent = "";
  const totals = weeklyTotals(rows);
  const max = Math.max(1, ...totals.map((t) => t.km));
  for (const t of totals) {
    const row = document.createElement("div");
    row.className = "bar-row";
    const bar = document.createElement("div");
    bar.className = "bar";
    bar.style.width = `${Math.max(2, (t.km / max) * 100)}%`;
    const label = document.createElement("span");
    label.textContent = `${t.week}: ${t.km.toFixed(1)} km`;
    row.append(bar, label);
    chart.appendChild(row);
  }
  if (!totals.length) chart.textContent = "No dated sessions yet.";
}

async function refreshHistory(): Promise<void> {
  try {
    const resp = await api.sessions(20);
    renderSessions(resp.sessions);
    renderWeekly(resp.sessions);
  } catch { /* next poll retries */ }
}

async function pollStatus(): Promise<void> {
  try {
    const s = await api.status();
    setConnected(true);
    renderStatus(s);
    if (pollTimer !== undefined) window.clearTimeout(pollTimer);
    pollTimer = window.setTimeout(pollStatus, 1000); /* ≤ 1 s refresh */
  } catch {
    setConnected(false);
    if (pollTimer !== undefined) window.clearTimeout(pollTimer);
    pollTimer = window.setTimeout(pollStatus, 3000);
  }
}

async function onToggle(): Promise<void> {
  const out = $("toggle-result");
  try {
    const r = await api.toggle();
    out.textContent = r.ok && r.data ? `Device state: ${r.data.state}` : "Toggle failed.";
    pollStatus();
  } catch {
    out.textContent = "Toggle failed — device unreachable.";
  }
}

function setInput(id: string, value: string): void {
  const el = document.getElementById(id) as HTMLInputElement | HTMLSelectElement | null;
  if (el) el.value = value;
}

function fillConfigDefaults(): void {
  /* Defaults mirror firmware cfg_default(); the device is authoritative on
   * save-rejection, so this is a convenience not a source of truth. */
  setInput("f-circ", "2135");
  setInput("f-gear-num", "1");
  setInput("f-gear-den", "1");
  setInput("f-debounce", "30");
  setInput("f-ema", "25");
  setInput("f-dropout", "750");
  setInput("f-units", "0");
  setInput("f-full", "30");
}

async function onConfigSubmit(ev: Event): Promise<void> {
  ev.preventDefault();
  const out = $("config-result");
  const num = (id: string) => Number((document.getElementById(id) as HTMLInputElement).value);
  const body = {
    circumference_mm: num("f-circ"),
    gear_num: num("f-gear-num"),
    gear_den: num("f-gear-den"),
    debounce_ms: num("f-debounce"),
    ema_percent: num("f-ema"),
    dropout_ms: num("f-dropout"),
    units: Number((document.getElementById("f-units") as HTMLSelectElement).value),
    full_every: num("f-full"),
  };
  const r = await api.config(body);
  if (r.ok) {
    out.textContent = "Settings saved on device.";
    return;
  }
  const err = r.error;
  const fieldToInput: Record<string, string> = {
    circumference_mm: "f-circ", gear_ratio: "f-gear-num",
    debounce_ms: "f-debounce", ema_percent: "f-ema",
    dropout_ms: "f-dropout", units: "f-units", full_every: "f-full",
  };
  if (err && err.field) {
    const target = fieldToInput[err.field];
    const msg = `${err.field}: ${err.reason}`;
    if (target === "f-circ") {
      const e = $("err-circ");
      e.textContent = msg;
      e.hidden = false;
      document.getElementById("f-circ")?.focus();
    } else {
      document.getElementById(target ?? "f-circ")?.focus();
    }
    out.textContent = `Rejected by device — ${msg}`;
  } else {
    out.textContent = "Rejected by device (unknown field).";
  }
}

/* Wipe flow per protocol: fetch nonce immediately before the call, single use. */
async function onWipe(): Promise<void> {
  const out = $("wipe-result");
  if (!window.confirm("Erase ALL stored sessions from the device? This cannot be undone.")) {
    out.textContent = "Wipe cancelled.";
    return;
  }
  try {
    const n = await api.wipeNonce();
    const r = await api.wipe(n.nonce);
    out.textContent = r.ok ? "All session data erased." : `Wipe rejected (${r.status}).`;
    refreshHistory();
  } catch {
    out.textContent = "Wipe failed — device unreachable.";
  }
}

function wire(): void {
  for (const t of tabs) {
    $(`tab-${t}`).addEventListener("click", () => {
      showTab(t);
      if (t === "history") refreshHistory();
      if (t === "settings") fillConfigDefaults();
    });
  }
  $("btn-toggle").addEventListener("click", onToggle);
  ($("config-form") as HTMLFormElement).addEventListener("submit", onConfigSubmit);
  $("btn-wipe").addEventListener("click", onWipe);
}

wire();
pollStatus();
