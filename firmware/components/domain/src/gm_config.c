#include <string.h>
#include "gm_config.h"

void cfg_default(cfg_t *c)
{
    memset(c, 0, sizeof(*c));
    c->circ_mm = 2135;      /* 700c chalk-and-tape ballpark; user verifies */
    c->gear_num = 1;
    c->gear_den = 1;
    c->debounce_ms = 30;    /* architecture §6 default */
    c->ema_pct = 25;
    c->dropout_ms = 750;    /* architecture §6 default */
    c->units = 0;
    c->full_every = 30;     /* architecture §6 ghosting default */
}

int cfg_validate(const cfg_t *c, cfg_err_t *err)
{
#define BAD(f, r) do { if (err) { err->field = (f); err->reason = (r); } return -1; } while (0)
    if (c->circ_mm < CFG_CIRC_MIN || c->circ_mm > CFG_CIRC_MAX)
        BAD("circumference_mm", "out_of_range");
    if (c->gear_num == 0 || c->gear_den == 0 || c->gear_num > 99 || c->gear_den > 99)
        BAD("gear_ratio", "invalid");
    if (c->debounce_ms < CFG_DEB_MIN || c->debounce_ms > CFG_DEB_MAX)
        BAD("debounce_ms", "out_of_range");
    if (c->ema_pct < CFG_EMA_MIN || c->ema_pct > CFG_EMA_MAX)
        BAD("ema_percent", "out_of_range");
    if (c->dropout_ms < CFG_DROP_MIN || c->dropout_ms > CFG_DROP_MAX)
        BAD("dropout_ms", "out_of_range");
    if (c->units > 1) BAD("units", "invalid");
    if (c->full_every == 0 || c->full_every > 250) BAD("full_every", "out_of_range");
    /* ssid/pass shape: non-emptiness enforced by the captive-portal flow,
     * length caps are structurally enforced by the fixed buffers. */
    return 0;
#undef BAD
}
