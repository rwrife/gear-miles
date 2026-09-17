import type {
  ConfigBody, ConfigError, NonceResponse, SessionsResponse,
  SessionDetail, StatusResponse, ToggleResponse, WipeResponse,
} from "./types";

/* Same-origin fetches only — the dashboard talks exclusively to the device
 * (docs/protocol.md). No absolute URLs are allowed anywhere in this code
 * (CI asset audit enforces the bundle has no external hosts). */

async function getJson<T>(path: string): Promise<T> {
  const res = await fetch(path, { headers: { accept: "application/json" } });
  if (!res.ok) throw new ApiError(res.status, await safeText(res));
  return (await res.json()) as T;
}

interface PostResult<T> {
  ok: boolean;
  status: number;
  data?: T;
  error?: ConfigError;
}

async function postJson<T>(path: string, body?: string): Promise<PostResult<T>> {
  const res = await fetch(path, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: body ?? "",
  });
  const text = await safeText(res);
  if (res.status === 400 || res.status === 403) {
    let err: ConfigError | undefined;
    try { err = JSON.parse(text) as ConfigError; } catch { /* not JSON */ }
    return { ok: false, status: res.status, error: err };
  }
  if (!res.ok) throw new ApiError(res.status, text);
  return { ok: true, status: res.status, data: JSON.parse(text) as T };
}

async function safeText(res: Response): Promise<string> {
  try { return await res.text(); } catch { return ""; }
}

export class ApiError extends Error {
  status: number;
  constructor(status: number, body: string) {
    super(`HTTP ${status}: ${body.slice(0, 200)}`);
    this.name = "ApiError";
    this.status = status;
  }
}

export const api = {
  status: () => getJson<StatusResponse>("/api/status"),
  sessions: (limit = 20) => getJson<SessionsResponse>(`/api/sessions?limit=${limit}`),
  sessionDetail: (id: number) => getJson<SessionDetail>(`/api/sessions/${id}`),
  toggle: () => postJson<ToggleResponse>("/api/session/toggle"),
  config: (body: ConfigBody) =>
    postJson<{ ok: true }>("/api/config", JSON.stringify(body)),
  wipeNonce: () => getJson<NonceResponse>("/api/wipe/nonce"),
  wipe: (nonce: number) => postJson<WipeResponse>("/api/data/wipe", JSON.stringify({ nonce })),
};
