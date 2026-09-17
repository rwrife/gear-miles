/* Protocol types — mirrors docs/protocol.md v1 and the firmware responses
 * (firmware/components/domain/src/gm_api.c). Unknown fields must be
 * ignored (compatibility rule); all estimated values carry estimate:true
 * plus estimate_basis labeling (metric-honesty policy). */

export interface StatusResponse {
  v: number;
  state: "idle" | "running" | "paused" | "finished" | "settings" | "factory_reset_confirm";
  session_id: number;
  elapsed_s: number;
  distance_km: number;
  speed_kmh: number;
  cadence_rpm: number;
  estimate: true;
  estimate_basis: string;
  history_slots_free: number;
  dropped_records: number;
  wifi_connected: boolean;
  display_error: boolean;
  uptime_s: number;
}

export interface SessionSummary {
  id: number;
  started_epoch: number;
  elapsed_s: number;
  distance_km: number;
  avg_rpm: number;
  max_rpm: number;
  estimate: true;
}

export interface SessionsResponse {
  v: number;
  schema_version: number;
  estimate: true;
  estimate_basis: string;
  sessions: SessionSummary[];
}

export interface SessionDetail extends SessionSummary {
  v: number;
  schema_version: number;
  estimate_basis: string;
}

export interface ConfigBody {
  circumference_mm?: number;
  gear_num?: number;
  gear_den?: number;
  debounce_ms?: number;
  ema_percent?: number;
  dropout_ms?: number;
  units?: number;
  full_every?: number;
}

/* Machine-readable rejection from firmware validation. */
export interface ConfigError {
  field: string;
  reason: string;
}

export interface ToggleResponse {
  state: StatusResponse["state"];
}

export interface NonceResponse {
  nonce: number;
  ttl_note: string;
}

export interface WipeResponse {
  wiped: boolean;
}
