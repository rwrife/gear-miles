#include "gm_odometer.h"

void odo_init(odo_t *o)
{
    o->dist_mm = 0;
    o->rem_num = 0;
}

void odo_integrate(odo_t *o, uint32_t rpm, uint32_t dt_ms,
                   uint32_t circ_mm, uint32_t gear_num, uint32_t gear_den)
{
    if (gear_den == 0) gear_den = 1;
    if (rpm == 0 || dt_ms == 0 || circ_mm == 0) return;
    /* mm = rpm rev/min / 60 * gear * circ_mm * dt_ms/1000
     *    = rpm * num * circ_mm * dt_ms / (den * 60000) */
    uint64_t num = (uint64_t)rpm * gear_num * circ_mm * dt_ms + o->rem_num;
    uint64_t den = (uint64_t)gear_den * 60000ull;
    o->dist_mm += num / den;
    o->rem_num = num % den;
}

uint32_t odo_speed_dkmh(uint32_t rpm, uint32_t circ_mm,
                        uint32_t gear_num, uint32_t gear_den)
{
    if (gear_den == 0) gear_den = 1;
    uint64_t num = (uint64_t)rpm * gear_num * circ_mm * 3ull;
    uint64_t den = (uint64_t)gear_den * 5000ull;
    return (uint32_t)((num + den / 2) / den);
}
