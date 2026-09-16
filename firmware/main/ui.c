/* E-ink UI: renders session state + estimates with the honesty labeling
 * from architecture §7 (asterisk + legend in this compact surface).
 * Refresh budget enforcement lives inside gm_panel (<= 1 Hz partial, full
 * every N); ui_update just draws and calls the panel API every second. */
#include <stdio.h>
#include <string.h>
#include "gm_api.h"
#include "gm_panel.h"
#include "ui.h"

static char fmt_dkmh(uint32_t dkmh, char *buf, size_t cap)
{
    snprintf(buf, cap, "%u.%u", (unsigned)(dkmh / 10u), (unsigned)(dkmh % 10u));
    return buf[0];
}

void ui_draw(panel_t *p, const api_ctx_t *ctx)
{
    char b[24];
    panel_fill(p, 0xFF); /* white */

    /* header: state */
    panel_draw_str(p, 4, 4, "GEAR MILES", 0);
    panel_draw_str(p, 4, 20, sm_state_name(ctx->sm.state), 1);

    /* cadence + speed */
    snprintf(b, sizeof(b), "%u RPM*", (unsigned)ctx->cad.rpm);
    panel_draw_str(p, 4, 40, b, 0);
    char sp[12];
    fmt_dkmh(odo_speed_dkmh(ctx->cad.rpm, ctx->cfg.circ_mm, ctx->cfg.gear_num,
                            ctx->cfg.gear_den), sp, sizeof(sp));
    snprintf(b, sizeof(b), "%s KM/H*", sp);
    panel_draw_str(p, 4, 56, b, 0);

    /* distance (mm -> km with 2 decimals) */
    snprintf(b, sizeof(b), "%u.%02u KM*",
             (unsigned)(ctx->odo.dist_mm / 1000000ull),
             (unsigned)((ctx->odo.dist_mm / 10000ull) % 100));
    panel_draw_str(p, 4, 72, b, 0);

    /* elapsed */
    snprintf(b, sizeof(b), "%02u:%02u", (unsigned)(ctx->session_elapsed_s / 60),
             (unsigned)(ctx->session_elapsed_s % 60));
    panel_draw_str(p, 4, 88, b, 0);

    /* honesty legend (§7 rule 2: compact surface = asterisk + legend) */
    panel_draw_str(p, 4, 108, "*EST. CADENCE X CIRC", 0);

    /* dropout hint (arch §6 "sensor?") */
    if (ctx->sm.state == ST_PAUSED)
        panel_draw_str(p, 150, 88, "SENSOR?", 0);
}
