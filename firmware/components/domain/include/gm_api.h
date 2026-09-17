/* Gear Miles — LAN HTTP API handlers (pure C, host-testable).
 *
 * Endpoint semantics follow docs/protocol.md (draft v0): /api/status,
 * /api/sessions, session start/stop (mirrors buttons), config validate-
 * and-set, nonce-gated wipe, CSV/JSON export. The ESP-IDF http server
 * glue in main/ maps sockets onto these functions; all routing/payload
 * logic is testable here without a network stack.
 *
 * Metric-honesty policy (architecture §7): speed/distance fields carry
 * sibling "estimate": true and estimate_basis — enforced in one place,
 * here.
 */
#ifndef GM_API_H
#define GM_API_H

#include <stddef.h>
#include "gm_cadence.h"
#include "gm_config.h"
#include "gm_odometer.h"
#include "gm_ring.h"
#include "gm_session_sm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define API_PROTO_V 1 /* frozen protocol v1 (docs/protocol.md, issue #7) */

typedef struct {
    cfg_t     cfg;
    sm_t      sm;
    rs_t     *ring;
    cad_t     cad;
    odo_t     odo;
    uint32_t  session_started_epoch;
    uint32_t  session_elapsed_s;   /* frozen while paused */
    uint32_t  running_since_ms;    /* 0 = not actively integrating */
    uint16_t  session_max_rpm;
    uint64_t  session_dist_start_mm;
    uint32_t  uptime_ms;           /* monotonic, host-injected */
    uint32_t  nonce;               /* active wipe nonce, single use */
    uint32_t  rng_state;           /* xorshift state (boot-seeded) */
    uint8_t   wifi_connected;
    uint8_t   display_error;
} api_ctx_t;

typedef struct {
    int  status;               /* HTTP status */
    char body[1024];
    const char *content_type;
} api_resp_t;

void api_init(api_ctx_t *ctx, rs_t *ring, uint32_t boot_seed);
uint32_t api_rng(api_ctx_t *ctx);

/* GET /api/status */
void api_status(api_ctx_t *ctx, api_resp_t *r);
/* GET /api/sessions?limit=N */
void api_sessions(api_ctx_t *ctx, uint32_t limit, api_resp_t *r);
/* GET /api/sessions/<id> */
void api_session_detail(api_ctx_t *ctx, uint32_t id, api_resp_t *r);
/* GET /export.csv | /export.json */
void api_export_csv(api_ctx_t *ctx, api_resp_t *r);
/* POST /api/session/start | /api/session/stop (mirrors BTN_A) */
void api_session_toggle(api_ctx_t *ctx, api_resp_t *r);
/* Periodic pump (call from the 1 Hz UI task): applies time-driven events
 * (elapsed integration, dropout auto-pause, FINISHED/SETTINGS timeouts). */
void api_tick(api_ctx_t *ctx, uint32_t now_ms);
/* POST /api/config — flat JSON object body.
 * Unknown keys are ignored; first validation failure -> 400 + field/reason.
 * Only applied after full validation (all-or-nothing). */
void api_config_post(api_ctx_t *ctx, const char *body, api_resp_t *r);
/* GET /api/wipe/nonce — short-lived single-use nonce */
void api_wipe_nonce(api_ctx_t *ctx, api_resp_t *r);
/* POST /api/data/wipe {"nonce":N} — must match the active nonce. */
void api_wipe_post(api_ctx_t *ctx, const char *body, api_resp_t *r);

/* Minimal flat-JSON object scanner used by the POST handlers:
 * {"k":num|"string", ...}; calls cb for each pair. Returns 0 on parse
 * success. No nesting (documented API constraint). */
typedef void (*api_json_cb)(void *u, const char *key, int is_str,
                            double num, const char *str, size_t str_len);
int api_json_each(const char *body, api_json_cb cb, void *u);

#ifdef __cplusplus
}
#endif
#endif /* GM_API_H */
