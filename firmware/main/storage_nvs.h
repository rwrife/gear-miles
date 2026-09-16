#ifndef GM_STORAGE_NVS_H
#define GM_STORAGE_NVS_H
#include "gm_config.h"
void nvs_cfg_load(cfg_t *c);
int  nvs_cfg_store(const cfg_t *c);
int  nvs_cfg_wipe(void);
#endif
