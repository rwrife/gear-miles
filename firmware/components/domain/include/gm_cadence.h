/* Gear Miles — cadence pipeline (pure C, host-testable).
 *
 * Debounce + EMA + dropout timeout per docs/architecture.md §6 and
 * firmware/README.md. No ESP-IDF dependencies: all time enters through
 * injected timestamps so every rule is unit-testable on a host.
 */
#ifndef GM_CADENCE_H
#define GM_CADENCE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t debounce_ms;   /* edges closer than this are ignored (never counted) */
    uint32_t ema_alpha_pct; /* weight of newest sample, percent 1..100 */
    uint32_t dropout_ms;    /* after this much silence the read shows 0 */
} cad_cfg_t;

typedef struct {
    cad_cfg_t cfg;
    uint32_t last_edge_ms;    /* last accepted edge (0 = none yet) */
    uint32_t has_edge;
    uint32_t ema_rpm_x256;    /* EMA in 8.8 fixed point */
    uint32_t rpm;             /* current filtered reading (0 in dropout) */
    uint32_t dropped_edges;   /* diagnostics: edges swallowed by debounce */
} cad_t;

/* RPM ceiling for one raw sample; a reed bounce or noise burst must not
 * inject absurd samples (requirement E4: 0-200 RPM sensing range). */
#define CAD_RAW_RPM_CLAMP 300u

void cad_init(cad_t *c, const cad_cfg_t *cfg);

/* Sensor edge callback. now_ms = monotonic time of the edge.
 * Returns 1 if the edge was accepted, 0 if rejected by the debounce window
 * (architecture §6: "bounce < configured window ... ignored, never counted").
 */
int cad_edge(cad_t *c, uint32_t now_ms);

/* Periodic housekeeping (call at <= 1 Hz). Applies the dropout rule:
 * after cfg.dropout_ms of silence the reading is forced to 0 immediately
 * while the elapsed timer keeps running (architecture §6 row 1).
 */
void cad_tick(cad_t *c, uint32_t now_ms);

/* Seconds of continuous dropout (0 while cadence is live). Used by the
 * session state machine for the 5-minute auto-pause rule. */
uint32_t cad_dropout_secs(const cad_t *c, uint32_t now_ms);

#ifdef __cplusplus
}
#endif
#endif /* GM_CADENCE_H */
