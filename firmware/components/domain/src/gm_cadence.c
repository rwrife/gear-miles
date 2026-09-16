#include "gm_cadence.h"

static uint32_t rpm_from_gap(uint32_t gap_ms)
{
    if (gap_ms == 0) gap_ms = 1;
    uint64_t rpm = (60000ull + gap_ms / 2) / gap_ms; /* rounded */
    if (rpm > CAD_RAW_RPM_CLAMP) rpm = CAD_RAW_RPM_CLAMP;
    return (uint32_t)rpm;
}

void cad_init(cad_t *c, const cad_cfg_t *cfg)
{
    *c = (cad_t){0};
    c->cfg = *cfg;
    if (c->cfg.ema_alpha_pct == 0) c->cfg.ema_alpha_pct = 25;
}

int cad_edge(cad_t *c, uint32_t now_ms)
{
    if (c->has_edge) {
        uint32_t gap = now_ms - c->last_edge_ms; /* wraps are fine: monotonic */
        if (gap < c->cfg.debounce_ms) {
            c->dropped_edges++;
            return 0; /* bounce: ignored, never counted (§6) */
        }
        uint32_t rpm = rpm_from_gap(gap);
        uint32_t new_x256 = rpm * 256u;
        if (c->ema_rpm_x256 == 0) {
            c->ema_rpm_x256 = new_x256; /* first sample seeds the filter */
        } else {
            uint32_t a = c->cfg.ema_alpha_pct;
            c->ema_rpm_x256 = (new_x256 * a + c->ema_rpm_x256 * (100u - a)) / 100u;
        }
        c->rpm = (c->ema_rpm_x256 + 128u) / 256u;
    }
    c->last_edge_ms = now_ms;
    c->has_edge = 1;
    return 1;
}

void cad_tick(cad_t *c, uint32_t now_ms)
{
    if (!c->has_edge) return;
    if ((uint32_t)(now_ms - c->last_edge_ms) >= c->cfg.dropout_ms)
        c->rpm = 0; /* read 0 immediately; EMA state kept for fast recovery */
}

uint32_t cad_dropout_secs(const cad_t *c, uint32_t now_ms)
{
    if (!c->has_edge) return 0;
    uint32_t silence = (uint32_t)(now_ms - c->last_edge_ms);
    if (silence < c->cfg.dropout_ms) return 0;
    return (silence - c->cfg.dropout_ms) / 1000u;
}
