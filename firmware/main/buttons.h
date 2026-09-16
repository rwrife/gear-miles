#ifndef GM_BUTTONS_H
#define GM_BUTTONS_H
#include <stdint.h>
#include "gm_api.h"
void buttons_init(void);
void buttons_pump(api_ctx_t *ctx);
int  cadence_pop_edge_us(int64_t *out_us);
#endif
