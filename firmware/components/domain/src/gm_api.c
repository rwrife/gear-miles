#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gm_api.h"

#define ESTIMATE_BASIS "crank cadence x configured circumference"

void api_init(api_ctx_t *ctx, rs_t *ring, uint32_t boot_seed)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->ring = ring;
    cfg_default(&ctx->cfg);
    cad_cfg_t cc = { ctx->cfg.debounce_ms, ctx->cfg.ema_pct, ctx->cfg.dropout_ms };
    cad_init(&ctx->cad, &cc);
    odo_init(&ctx->odo);
    ctx->sm.state = ST_IDLE;
    ctx->rng_state = boot_seed ? boot_seed : 0x9E3779B9u;
}

uint32_t api_rng(api_ctx_t *ctx)
{
    uint32_t x = ctx->rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    ctx->rng_state = x;
    return x ? x : 1;
}

static void resp_json(api_resp_t *r, int status)
{
    r->status = status;
    r->content_type = "application/json";
}

void api_status(api_ctx_t *ctx, api_resp_t *r)
{
    snprintf(r->body, sizeof(r->body),
        "{\"v\":%d,"
        "\"state\":\"%s\","
        "\"session_id\":%u,"
        "\"elapsed_s\":%u,"
        "\"distance_km\":%.3f,"
        "\"speed_kmh\":%.1f,"
        "\"cadence_rpm\":%u,"
        "\"estimate\":true,"
        "\"estimate_basis\":\"%s\","
        "\"history_slots_free\":%u,"
        "\"dropped_records\":%u,"
        "\"wifi_connected\":%s,"
        "\"display_error\":%s,"
        "\"uptime_s\":%u}",
        API_PROTO_V,
        sm_state_name(ctx->sm.state),
        (unsigned)(ctx->ring ? (ctx->ring->seq_next > 0 ? ctx->ring->seq_next - 1 : 0) : 0),
        (unsigned)ctx->session_elapsed_s,
        (double)ctx->odo.dist_mm / 1000000.0,
        (double)odo_speed_dkmh(ctx->cad.rpm, ctx->cfg.circ_mm,
                               ctx->cfg.gear_num, ctx->cfg.gear_den) / 10.0,
        (unsigned)ctx->cad.rpm,
        ESTIMATE_BASIS,
        (unsigned)(ctx->ring ? rs_slots_free(ctx->ring) : 0),
        (unsigned)(ctx->ring ? ctx->ring->dropped : 0),
        ctx->wifi_connected ? "true" : "false",
        ctx->display_error ? "true" : "false",
        (unsigned)(ctx->uptime_ms / 1000u));
    resp_json(r, 200);
}

void api_sessions(api_ctx_t *ctx, uint32_t limit, api_resp_t *r)
{
    if (!ctx->ring) {
        resp_json(r, 500);
        snprintf(r->body, sizeof(r->body), "{\"error\":\"no_store\"}");
        return;
    }
    if (limit == 0 || limit > 50) limit = 20;
    uint32_t newest = ctx->ring->seq_next - 1; /* last appended seq */
    char *p = r->body;
    int rem = (int)sizeof(r->body);
    int used = snprintf(p, (size_t)rem,
        "{\"v\":%d,\"schema_version\":1,\"estimate\":true,"
        "\"estimate_basis\":\"%s\",\"sessions\":[", API_PROTO_V, ESTIMATE_BASIS);
    p += used; rem -= used;
    uint32_t emitted = 0;
    /* newest-first via age so an empty ring (newest==0) exits immediately */
    for (uint32_t age = 0; age < ctx->ring->slot_count && emitted < limit; age++) {
        if (age + 1 > newest) break;
        uint32_t seq = newest - age;
        rs_rec_t rec;
        if (rs_get(ctx->ring, seq, &rec) != 0) continue;
        used = snprintf(p, (size_t)rem,
            "%s{\"id\":%u,\"started_epoch\":%u,\"elapsed_s\":%u,"
            "\"distance_km\":%.3f,\"avg_rpm\":%u,\"max_rpm\":%u,\"estimate\":true}",
            emitted ? "," : "", (unsigned)rec.seq, (unsigned)rec.started_epoch, (unsigned)rec.elapsed_s,
            (double)rec.dist_mm / 1000000.0, rec.avg_rpm, rec.max_rpm);
        p += used; rem -= used; emitted++;
        if (rem < 128) break;
    }
    snprintf(p, (size_t)(rem > 2 ? rem : 2), "]}");
    resp_json(r, 200);
}

void api_session_detail(api_ctx_t *ctx, uint32_t id, api_resp_t *r)
{
    rs_rec_t rec;
    if (!ctx->ring || rs_get(ctx->ring, id, &rec) != 0) {
        resp_json(r, 404);
        snprintf(r->body, sizeof(r->body), "{\"error\":\"not_found\"}");
        return;
    }
    snprintf(r->body, sizeof(r->body),
        "{\"v\":%d,\"schema_version\":1,\"id\":%u,\"started_epoch\":%u,"
        "\"elapsed_s\":%u,\"distance_km\":%.3f,\"avg_rpm\":%u,\"max_rpm\":%u,"
        "\"estimate\":true,\"estimate_basis\":\"%s\"}",
        API_PROTO_V, (unsigned)rec.seq, (unsigned)rec.started_epoch, (unsigned)rec.elapsed_s,
        (double)rec.dist_mm / 1000000.0, rec.avg_rpm, rec.max_rpm,
        ESTIMATE_BASIS);
    resp_json(r, 200);
}

void api_export_csv(api_ctx_t *ctx, api_resp_t *r)
{
    if (!ctx->ring) { resp_json(r, 500); return; }
    char *p = r->body;
    int rem = (int)sizeof(r->body);
    int used = snprintf(p, (size_t)rem,
        "# schema_version=1\n"
        "# estimate_basis=%s\n"
        "id,started_epoch,elapsed_s,distance_km*,avg_rpm,max_rpm\n",
        ESTIMATE_BASIS);
    p += used; rem -= used;
    for (uint32_t seq = 1; seq < ctx->ring->seq_next; seq++) {
        rs_rec_t rec;
        if (rs_get(ctx->ring, seq, &rec) != 0) continue;
        used = snprintf(p, (size_t)rem, "%u,%u,%u,%.3f,%u,%u\n",
                        (unsigned)rec.seq, (unsigned)rec.started_epoch, (unsigned)rec.elapsed_s,
                        (double)rec.dist_mm / 1000000.0, rec.avg_rpm, rec.max_rpm);
        p += used; rem -= used;
        if (rem < 64) {
            snprintf(p, (size_t)(rem > 2 ? rem : 2), "# truncated\n");
            break;
        }
    }
    r->status = 200;
    r->content_type = "text/csv";
}

/* GET /export.json — additive v1.1 endpoint (filed issue #14). Payload is
 * the frozen sessions-list shape (docs/protocol.md: {"v","schema_version",
 * "estimate","estimate_basis","sessions":[...]}) over the full history,
 * oldest-first like the CSV export. Same 1 KiB response budget as CSV:
 * when records do not fit, the array closes cleanly and a sibling
 * "truncated":true flag is added (unknown-field rule makes it additive;
 * metric-honesty policy requires the user see incomplete exports). */
void api_export_json(api_ctx_t *ctx, api_resp_t *r)
{
    if (!ctx->ring) { resp_json(r, 500); return; }
    char *p = r->body;
    int rem = (int)sizeof(r->body);
    int used = snprintf(p, (size_t)rem,
        "{\"v\":%d,\"schema_version\":1,\"estimate\":true,"
        "\"estimate_basis\":\"%s\",\"sessions\":[",
        API_PROTO_V, ESTIMATE_BASIS);
    p += used; rem -= used;
    int truncated = 0, first = 1;
    for (uint32_t seq = 1; seq < ctx->ring->seq_next; seq++) {
        rs_rec_t rec;
        if (rs_get(ctx->ring, seq, &rec) != 0) continue;
        /* Worst-case record with comma (u32/u64-saturated fields) is 150 B;
         * closing `]` + `,"truncated":true}` is 21 B. Require 172 B free
         * before emitting a record so the JSON always closes valid. */
        if (rem < 172) { truncated = 1; break; }
        used = snprintf(p, (size_t)rem,
            "%s{\"id\":%u,\"started_epoch\":%u,\"elapsed_s\":%u,"
            "\"distance_km\":%.3f,\"avg_rpm\":%u,\"max_rpm\":%u,\"estimate\":true}",
            first ? "" : ",", (unsigned)rec.seq, (unsigned)rec.started_epoch,
            (unsigned)rec.elapsed_s,
            (double)rec.dist_mm / 1000000.0, rec.avg_rpm, rec.max_rpm);
        p += used; rem -= used; first = 0;
    }
    used = truncated
        ? snprintf(p, (size_t)rem, "],\"truncated\":true}")
        : snprintf(p, (size_t)rem, "]}");
    (void)used;
    resp_json(r, 200);
}

static void api_commit_session(api_ctx_t *ctx)
{
    if (!ctx->ring) return;
    rs_rec_t rec = {0};
    rec.started_epoch = ctx->session_started_epoch;
    rec.elapsed_s = ctx->session_elapsed_s;
    rec.dist_mm = ctx->odo.dist_mm;
    rec.avg_rpm = (uint16_t)ctx->cad.rpm; /* last reading; full avg is #8 tuning */
    rec.max_rpm = ctx->session_max_rpm;
    rec.estimate_basis = 1;
    rs_append(ctx->ring, &rec);
}

void api_session_toggle(api_ctx_t *ctx, api_resp_t *r)
{
    uint32_t act = sm_handle(&ctx->sm, EV_TAP_A, ctx->uptime_ms);
    if (act & SM_ACT_START_SESSION) {
        ctx->session_started_epoch = 0; /* SNTP cosmetic; monotonic authoritative */
        ctx->session_elapsed_s = 0;
        odo_init(&ctx->odo);
        ctx->session_max_rpm = 0;
        ctx->session_dist_start_mm = 0;
        ctx->running_since_ms = ctx->uptime_ms;
    }
    if (act & SM_ACT_STOP_TIMER) ctx->running_since_ms = 0;
    if (act & SM_ACT_RESUME_TIMER) ctx->running_since_ms = ctx->uptime_ms;
    if (act & SM_ACT_COMMIT_SESSION && ctx->ring) api_commit_session(ctx);
    resp_json(r, 200);
    snprintf(r->body, sizeof(r->body), "{\"state\":\"%s\"}",
             sm_state_name(ctx->sm.state));
}

void api_tick(api_ctx_t *ctx, uint32_t now_ms)
{
    uint32_t prev = ctx->uptime_ms;
    ctx->uptime_ms = now_ms;

    /* cadence housekeeping while running */
    cad_tick(&ctx->cad, now_ms);
    if (ctx->cad.rpm > ctx->session_max_rpm)
        ctx->session_max_rpm = (uint16_t)ctx->cad.rpm;

    /* integrate distance + elapsed while actively running */
    if (ctx->sm.state == ST_RUNNING && ctx->running_since_ms != 0 && now_ms > prev) {
        uint32_t dt = now_ms - prev;
        odo_integrate(&ctx->odo, ctx->cad.rpm, dt, ctx->cfg.circ_mm,
                      ctx->cfg.gear_num, ctx->cfg.gear_den);
        ctx->session_elapsed_s += dt / 1000u;
    }

    /* dropout auto-pause after 5 min continuous (arch §5/§6) */
    if (ctx->sm.state == ST_RUNNING &&
        cad_dropout_secs(&ctx->cad, now_ms) >= 300u) {
        uint32_t act = sm_handle(&ctx->sm, EV_DROPOUT_5MIN, now_ms);
        if (act & SM_ACT_STOP_TIMER) ctx->running_since_ms = 0;
        return;
    }
    /* FINISHED dwell 2 s -> commit (arch §5) */
    if (ctx->sm.state == ST_FINISHED &&
        now_ms - ctx->sm.state_since_ms >= 2000u) {
        uint32_t act = sm_handle(&ctx->sm, EV_FINISHED_TIMEOUT, now_ms);
        if (act & SM_ACT_COMMIT_SESSION) api_commit_session(ctx);
        return;
    }
    /* SETTINGS idle 60 s -> persist */
    if (ctx->sm.state == ST_SETTINGS &&
        now_ms - ctx->sm.state_since_ms >= 60000u) {
        sm_handle(&ctx->sm, EV_SETTINGS_TIMEOUT, now_ms);
    }
}

/* ---- config POST ---- */

typedef struct {
    cfg_t proposed;
    int seen_bad;
    char bad_field[32];
} cfg_parse_t;

static void cfg_cb(void *u, const char *key, int is_str, double num,
                   const char *str, size_t str_len)
{
    cfg_parse_t *st = u;
    if (st->seen_bad) return;
    if (!strcmp(key, "circumference_mm") && !is_str) st->proposed.circ_mm = (uint16_t)num;
    else if (!strcmp(key, "gear_num") && !is_str) st->proposed.gear_num = (uint8_t)num;
    else if (!strcmp(key, "gear_den") && !is_str) st->proposed.gear_den = (uint8_t)num;
    else if (!strcmp(key, "debounce_ms") && !is_str) st->proposed.debounce_ms = (uint16_t)num;
    else if (!strcmp(key, "ema_percent") && !is_str) st->proposed.ema_pct = (uint8_t)num;
    else if (!strcmp(key, "dropout_ms") && !is_str) st->proposed.dropout_ms = (uint16_t)num;
    else if (!strcmp(key, "units") && !is_str) st->proposed.units = (uint8_t)num;
    else if (!strcmp(key, "full_every") && !is_str) st->proposed.full_every = (uint8_t)num;
    else if (!strcmp(key, "ssid") && is_str) {
        if (str_len >= sizeof(st->proposed.ssid)) {
            st->seen_bad = 1;
            snprintf(st->bad_field, sizeof(st->bad_field), "ssid");
            return;
        }
        memcpy(st->proposed.ssid, str, str_len);
        st->proposed.ssid[str_len] = 0;
    } else if (!strcmp(key, "pass") && is_str) {
        if (str_len >= sizeof(st->proposed.pass)) {
            st->seen_bad = 1;
            snprintf(st->bad_field, sizeof(st->bad_field), "pass");
            return;
        }
        memcpy(st->proposed.pass, str, str_len);
        st->proposed.pass[str_len] = 0;
    }
    /* unknown keys: ignored per protocol compatibility rule */
}

void api_config_post(api_ctx_t *ctx, const char *body, api_resp_t *r)
{
    cfg_parse_t st;
    st.proposed = ctx->cfg;
    st.seen_bad = 0;
    st.bad_field[0] = 0;
    if (api_json_each(body, cfg_cb, &st) != 0) {
        resp_json(r, 400);
        snprintf(r->body, sizeof(r->body),
                 "{\"field\":\"body\",\"reason\":\"malformed_json\"}");
        return;
    }
    cfg_err_t err = {0, 0};
    if (st.seen_bad) {
        resp_json(r, 400);
        snprintf(r->body, sizeof(r->body), "{\"field\":\"%s\",\"reason\":\"too_long\"}",
                 st.bad_field);
        return;
    }
    if (cfg_validate(&st.proposed, &err) != 0) {
        resp_json(r, 400);
        snprintf(r->body, sizeof(r->body), "{\"field\":\"%s\",\"reason\":\"%s\"}",
                 err.field, err.reason);
        return;
    }
    ctx->cfg = st.proposed;
    /* re-derive runtime cadence config (single reseed point) */
    ctx->cad.cfg.debounce_ms = ctx->cfg.debounce_ms;
    ctx->cad.cfg.ema_alpha_pct = ctx->cfg.ema_pct;
    ctx->cad.cfg.dropout_ms = ctx->cfg.dropout_ms;
    resp_json(r, 200);
    snprintf(r->body, sizeof(r->body), "{\"ok\":true}");
}

/* ---- wipe with nonce ---- */

void api_wipe_nonce(api_ctx_t *ctx, api_resp_t *r)
{
    ctx->nonce = api_rng(ctx);
    resp_json(r, 200);
    snprintf(r->body, sizeof(r->body), "{\"nonce\":%u,\"ttl_note\":\"single-use\"}",
             (unsigned)ctx->nonce);
}

typedef struct { double v; } num_out_t;

static void nonce_cb(void *u, const char *key, int is_str, double num,
                     const char *s, size_t l)
{
    (void)key; (void)is_str; (void)s; (void)l;
    ((num_out_t *)u)->v = num;
}

void api_wipe_post(api_ctx_t *ctx, const char *body, api_resp_t *r)
{
    num_out_t u = { -1 };
    if (api_json_each(body, nonce_cb, &u) != 0) {
        resp_json(r, 400);
        snprintf(r->body, sizeof(r->body),
                 "{\"field\":\"body\",\"reason\":\"malformed_json\"}");
        return;
    }
    if (ctx->nonce == 0 || (uint32_t)u.v != ctx->nonce) {
        resp_json(r, 403);
        snprintf(r->body, sizeof(r->body), "{\"error\":\"nonce_mismatch\"}");
        return;
    }
    ctx->nonce = 0; /* single use */
    int ok = ctx->ring ? (rs_wipe(ctx->ring) == 0) : 1;
    resp_json(r, ok ? 200 : 500);
    snprintf(r->body, sizeof(r->body), "{\"wiped\":%s}", ok ? "true" : "false");
}

/* ---- flat JSON object scanner ---- */
static const char *skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

int api_json_each(const char *body, api_json_cb cb, void *u)
{
    const char *p = skip_ws(body);
    if (*p != '{') return -1;
    p = skip_ws(p + 1);
    if (*p == '}') return 0;
    for (;;) {
        p = skip_ws(p);
        if (*p != '"') return -1;
        const char *k0 = ++p;
        while (*p && *p != '"') p++;
        if (*p != '"') return -1;
        size_t klen = (size_t)(p - k0);
        char key[32];
        if (klen >= sizeof(key)) return -1;
        memcpy(key, k0, klen);
        key[klen] = 0;
        p = skip_ws(p + 1);
        if (*p != ':') return -1;
        p = skip_ws(p + 1);
        if (*p == '"') {
            const char *v0 = ++p;
            while (*p && *p != '"') { if (*p == '\\') p++; p++; }
            if (*p != '"') return -1;
            cb(u, key, 1, 0, v0, (size_t)(p - v0));
            p = skip_ws(p + 1);
        } else if (!strncmp(p, "true", 4)) {
            cb(u, key, 0, 1, NULL, 0);
            p = skip_ws(p + 4);
        } else if (!strncmp(p, "false", 5)) {
            cb(u, key, 0, 0, NULL, 0);
            p = skip_ws(p + 5);
        } else if (!strncmp(p, "null", 4)) {
            cb(u, key, 0, 0, NULL, 0);
            p = skip_ws(p + 4);
        } else {
            char *end;
            double v = strtod(p, &end);
            if (end == p) return -1;
            cb(u, key, 0, v, NULL, 0);
            p = skip_ws(end);
        }
        if (*p == ',') { p++; continue; }
        if (*p == '}') return 0;
        return -1;
    }
}
