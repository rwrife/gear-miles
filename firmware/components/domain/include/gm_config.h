/* Gear Miles — configuration model + validation (pure C, host-testable).
 *
 * All server-side writes are validated with machine-readable
 * field/reason on rejection (docs/protocol.md "Semantics").
 */
#ifndef GM_CONFIG_H
#define GM_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     ssid[33];        /* max 32 chars + NUL; only set via captive flow */
    char     pass[65];        /* max 64 + NUL; stored in NVS only, never logged */
    uint16_t circ_mm;         /* effective circumference, mm */
    uint8_t  gear_num;        /* wheel revs per crank rev, numerator   */
    uint8_t  gear_den;        /* wheel revs per crank rev, denominator */
    uint16_t debounce_ms;     /* cadence debounce window */
    uint8_t  ema_pct;         /* EMA alpha percent for cadence filter */
    uint16_t dropout_ms;      /* cadence dropout timeout */
    uint8_t  units;           /* 0 = km/kmh, 1 = mi/mph */
    uint8_t  full_every;      /* forced full refresh every N partials */
} cfg_t;

typedef struct {
    const char *field;  /* machine-readable field name */
    const char *reason; /* machine-readable reason */
} cfg_err_t;

/* Factory defaults (architecture §6 defaults: debounce 30 ms,
 * dropout 750 ms, full refresh every 30 partials; 700c wheel-ish
 * default circumference 2135 mm, direct-drive ratio 1/1). */
void cfg_default(cfg_t *c);

/* Returns 0 when valid; -1 when invalid with *err filled. */
int cfg_validate(const cfg_t *c, cfg_err_t *err);

/* Ranges (exported for the API layer to publish). */
#define CFG_CIRC_MIN 300u
#define CFG_CIRC_MAX 4000u
#define CFG_DEB_MIN 5u
#define CFG_DEB_MAX 100u
#define CFG_EMA_MIN 5u
#define CFG_EMA_MAX 100u
#define CFG_DROP_MIN 100u
#define CFG_DROP_MAX 5000u

#ifdef __cplusplus
}
#endif
#endif /* GM_CONFIG_H */
