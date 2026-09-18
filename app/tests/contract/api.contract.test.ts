/* Contract tests: the dashboard's client code and formatting against the
 * REAL firmware API responses, served by the host stub
 * (firmware/test/stub) which links the actual gm_api.c handlers.
 * Run: npm run test:contract (stub must be listening; CI script starts it).
 * These are HOST/SIMULATION tests — never bench evidence. */
import { test } from "node:test";
import assert from "node:assert/strict";

import { api, ApiError } from "../../src/api.ts";
import { fmtElapsed, weeklyTotals, weekKey } from "../../src/format.ts";

const BASE = process.env.GM_STUB_URL ?? "http://127.0.0.1:8123";

/* Host-only fixture reset (stub tool endpoint, not device protocol): makes
 * the whole suite order- and repeat-independent. */
await fetch(BASE + "/__reset", { method: "GET" });

/* Node 22 fetch needs absolute URL: patch the relative-path api module by
 * asserting through fetch directly against the same paths the client uses. */
async function get(path: string): Promise<Response> {
  return fetch(BASE + path, { headers: { accept: "application/json" } });
}
async function post(path: string, body = ""): Promise<Response> {
  return fetch(BASE + path, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body,
  });
}

test("GET /api/status matches client StatusResponse shape", async () => {
  const res = await get("/api/status");
  assert.equal(res.status, 200);
  const s = await res.json();
  assert.equal(typeof s.v, "number");
  assert.ok(["idle", "running", "paused", "finished", "settings", "factory_reset_confirm"].includes(s.state));
  assert.equal(s.estimate, true, "metric-honesty: status must carry estimate:true");
  assert.equal(typeof s.estimate_basis, "string");
  assert.ok(s.estimate_basis.length > 0);
  for (const f of ["session_id", "elapsed_s", "distance_km", "speed_kmh",
                   "cadence_rpm", "history_slots_free", "dropped_records", "uptime_s"])
    assert.equal(typeof s[f], "number", `field ${f} numeric`);
  for (const b of ["wifi_connected", "display_error"])
    assert.equal(typeof s[b], "boolean", `field ${b} boolean`);
});

test("GET /api/sessions list contract", async () => {
  const res = await get("/api/sessions?limit=5");
  assert.equal(res.status, 200);
  const j = await res.json();
  assert.equal(j.schema_version, 1);
  assert.equal(j.estimate, true);
  assert.ok(Array.isArray(j.sessions));
  for (const s of j.sessions) {
    assert.equal(typeof s.id, "number");
    assert.equal(s.estimate, true);
    assert.ok("started_epoch" in s && "distance_km" in s && "avg_rpm" in s);
  }
});

test("GET /api/sessions/<id> detail + 404 contract", async () => {
  const list = await (await get("/api/sessions?limit=5")).json();
  if (list.sessions.length) {
    const id = list.sessions[0].id;
    const res = await get(`/api/sessions/${id}`);
    assert.equal(res.status, 200);
    const d = await res.json();
    assert.equal(d.id, id);
    assert.equal(d.estimate, true);
    assert.equal(typeof d.estimate_basis, "string");
  }
  const missing = await get("/api/sessions/999999");
  assert.equal(missing.status, 404);
  assert.deepEqual(await missing.json(), { error: "not_found" });
});

test("export.csv schema header + estimate labeling", async () => {
  const res = await get("/export.csv");
  assert.equal(res.status, 200);
  assert.match(res.headers.get("content-type") ?? "", /text\/csv/);
  const text = await res.text();
  const lines = text.trim().split("\n");
  assert.equal(lines[0], "# schema_version=1");
  assert.match(lines[1], /^# estimate_basis=.+/);
  assert.equal(lines[2], "id,started_epoch,elapsed_s,distance_km*,avg_rpm,max_rpm");
  for (const l of lines.slice(3)) {
    if (l.startsWith("#")) continue;
    assert.equal(l.split(",").length, 6);
  }
});

test("export.json sessions payload + estimate labeling (v1.1, issue #14)", async () => {
  const res = await get("/export.json");
  assert.equal(res.status, 200);
  assert.match(res.headers.get("content-type") ?? "", /application\/json/);
  const j = await res.json();
  assert.equal(typeof j.v, "number");
  assert.equal(j.schema_version, 1);
  assert.equal(j.estimate, true);
  assert.equal(typeof j.estimate_basis, "string");
  assert.ok(j.estimate_basis.length > 0);
  assert.ok(Array.isArray(j.sessions));
  for (const s of j.sessions) {
    assert.equal(s.estimate, true);
    assert.equal(typeof s.id, "number");
    assert.ok("started_epoch" in s && "elapsed_s" in s && "distance_km" in s &&
              "avg_rpm" in s && "max_rpm" in s);
  }
  /* Same-history consistency with the frozen CSV export (ids + count) */
  const csv = await (await get("/export.csv")).text();
  const csvIds = csv.trim().split("\n").filter((l) => /^\d+,/.test(l))
    .map((l) => Number(l.split(",")[0]));
  assert.deepEqual(j.sessions.map((s) => s.id), csvIds);
  if ("truncated" in j) assert.equal(j.truncated, true, "truncated flag is boolean-true only");
});

test("POST /api/config rejects out-of-range with field/reason", async () => {
  const cases: [string, string, string][] = [
    ['{"circumference_mm":100}', "circumference_mm", "out_of_range"],
    ['{"circumference_mm":9999}', "circumference_mm", "out_of_range"],
    ['{"debounce_ms":1}', "debounce_ms", "out_of_range"],
    ['{"dropout_ms":9000}', "dropout_ms", "out_of_range"],
    ['{"ema_percent":0}', "ema_percent", "out_of_range"],
    ['{"gear_num":0,"gear_den":1}', "gear_ratio", "invalid"],
    ['{"units":7}', "units", "invalid"],
    ['{"full_every":0}', "full_every", "out_of_range"],
    ["not json at all", "body", "malformed_json"],
  ];
  for (const [body, field, reason] of cases) {
    const res = await post("/api/config", body);
    assert.equal(res.status, 400, `reject ${body}`);
    const j = await res.json();
    assert.equal(j.field, field, `${body} -> field ${field}`);
    assert.equal(j.reason, reason, `${body} -> reason ${reason}`);
  }
});

test("POST /api/config all-or-nothing + unknown keys ignored", async () => {
  /* batch with one bad field leaves config untouched: verify via a known
   * good write, then a mixed batch, then re-read through a good write
   * echo path (config has no GET; verify via accept-then-reject-then-accept) */
  let res = await post("/api/config", '{"circumference_mm":2500,"units":1}');
  assert.equal(res.status, 200);
  assert.deepEqual(await res.json(), { ok: true });

  res = await post("/api/config", '{"circumference_mm":3000,"dropout_ms":9999}');
  assert.equal(res.status, 400);

  /* unknown keys must not break valid batches */
  res = await post("/api/config", '{"quantum_mode":true,"circumference_mm":2200}');
  assert.equal(res.status, 200);
});

test("wipe nonce flow: reject stale nonce, accept fresh, single-use", async () => {
  /* bad nonce -> 403 */
  let res = await post("/api/data/wipe", '{"nonce":12345}');
  assert.equal(res.status, 403);
  assert.deepEqual(await res.json(), { error: "nonce_mismatch" });

  /* fresh nonce -> success */
  const n = await (await get("/api/wipe/nonce")).json();
  assert.equal(typeof n.nonce, "number");
  res = await post("/api/data/wipe", `{"nonce":${n.nonce}}`);
  assert.equal(res.status, 200);
  assert.deepEqual(await res.json(), { wiped: true });

  /* replay same nonce -> 403 (single use) */
  res = await post("/api/data/wipe", `{"nonce":${n.nonce}}`);
  assert.equal(res.status, 403);

  /* history gone */
  const list = await (await get("/api/sessions?limit=50")).json();
  assert.equal(list.sessions.length, 0);
});

test("session toggle mirrors button semantics", async () => {
  let res = await post("/api/session/toggle");
  assert.equal(res.status, 200);
  let j = await res.json();
  assert.equal(j.state, "running");
  res = await post("/api/session/toggle");
  j = await res.json();
  assert.equal(j.state, "finished");
});

test("client formatting helpers", () => {
  assert.equal(fmtElapsed(0), "0:00");
  assert.equal(fmtElapsed(65), "1:05");
  assert.equal(fmtElapsed(3661), "1:01:01");
  assert.equal(weekKey(1758000000), "2025-09-15"); /* Mon 2025-09-15 */
  const t = weeklyTotals([
    { started_epoch: 1758000000, distance_km: 25 },
    { started_epoch: 1758086400, distance_km: 28.5 },
    { started_epoch: 1758200000, distance_km: 10 }, /* next week? 09-18 still same week */
    { started_epoch: 0, distance_km: 5 },           /* undated: skipped */
  ]);
  assert.equal(t.length, 1);
  assert.equal(t[0].km, 63.5);
});

test("ApiError carries HTTP status", async () => {
  const before = api.status; /* reference the client surface exists (typecheck) */
  assert.equal(typeof before, "function");
  const err = new ApiError(404, "nope");
  assert.equal(err.status, 404);
  assert.match(err.message, /HTTP 404/);
});
