/* Gear Miles — odometer math (pure C, integer-only, host-testable).
 *
 * distance += cadence/60 x crank_revs_to_wheel x configured effective
 * circumference, integrated per tick (firmware/README.md item 2).
 * Every output is an ESTIMATE from crank cadence x configured circumference
 * (metric-honesty policy, architecture §7) — the caller must label it.
 */
#ifndef GM_ODOMETER_H
#define GM_ODOMETER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t dist_mm;     /* total integrated distance, millimetres */
    uint64_t rem_num;     /* integration remainder (exact fixed point) */
} odo_t;

void odo_init(odo_t *o);

/* Integrate one step of dt_ms at rpm over a crank->wheel gear ratio
 * gear_num/gear_den (wheel revolutions per crank revolution).
 *
 * Exact integer math: dist_mm += rpm * circ_mm * dt_ms * num /
 * (den * 60000), with the division remainder carried in rem_num so
 * small steps do not bias low.
 */
void odo_integrate(odo_t *o, uint32_t rpm, uint32_t dt_ms,
                   uint32_t circ_mm, uint32_t gear_num, uint32_t gear_den);

/* Instantaneous speed estimate, km/h scaled by 10 (integer decikm/h).
 * km/h = rpm/60 * gear_ratio * circ_mm / 1e6 * 3600
 * km/h*10 = rpm * gear_num * circ_mm * 3 / (gear_den * 5000)
 */
uint32_t odo_speed_dkmh(uint32_t rpm, uint32_t circ_mm,
                        uint32_t gear_num, uint32_t gear_den);

#ifdef __cplusplus
}
#endif
#endif /* GM_ODOMETER_H */
