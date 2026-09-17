/* Pure formatting + aggregation helpers (exported for contract tests). */

export function fmtElapsed(s: number): string {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = Math.floor(s % 60);
  const mm = String(m).padStart(h > 0 ? 2 : 1, "0");
  const ss = String(sec).padStart(2, "0");
  return h > 0 ? `${h}:${mm}:${ss}` : `${m}:${ss}`;
}

export function fmtEpoch(epoch: number): string {
  if (!epoch) return "time unknown";
  return new Date(epoch * 1000).toLocaleString();
}

/* ISO week key (Monday-start, UTC) for the weekly trend chart. */
export function weekKey(epoch: number): string {
  const d = new Date(epoch * 1000);
  const day = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), d.getUTCDate()));
  const wd = (day.getUTCDay() + 6) % 7; /* Monday=0 */
  day.setUTCDate(day.getUTCDate() - wd);
  return day.toISOString().slice(0, 10);
}

export function weeklyTotals(
  sessions: { started_epoch: number; distance_km: number }[],
): { week: string; km: number }[] {
  const map = new Map<string, number>();
  for (const s of sessions) {
    if (!s.started_epoch) continue;
    const k = weekKey(s.started_epoch);
    map.set(k, (map.get(k) ?? 0) + s.distance_km);
  }
  return [...map.entries()]
    .sort((a, b) => a[0].localeCompare(b[0]))
    .map(([week, km]) => ({ week, km: Math.round(km * 100) / 100 }));
}
