#ifndef GM_WIFI_H
#define GM_WIFI_H
#include "gm_config.h"
void wifi_stack_init(cfg_t *cfg); /* event loop glue shared with main */
void wifi_setup_start(cfg_t *cfg); /* station join or captive portal */
#endif
