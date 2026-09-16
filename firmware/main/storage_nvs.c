/* cfg_t <-> NVS persistence (namespace "cfg"). SSID/pass stay in NVS only;
 * they are never sent to the LAN API and are redacted in logs (README §6). */
#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "gm_config.h"
#include "storage_nvs.h"

static const char *TAG = "cfg_nvs";
#define NS "cfg"

void nvs_cfg_load(cfg_t *c)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) != ESP_OK) {
        ESP_LOGI(TAG, "no stored config, using defaults");
        return;
    }
    cfg_t def = *c;
    uint16_t u16; uint8_t u8; size_t sz;
    if (nvs_get_u16(h, "circ", &u16) == ESP_OK) c->circ_mm = u16;
    if (nvs_get_u8(h, "gn", &u8) == ESP_OK) c->gear_num = u8;
    if (nvs_get_u8(h, "gd", &u8) == ESP_OK) c->gear_den = u8;
    if (nvs_get_u16(h, "deb", &u16) == ESP_OK) c->debounce_ms = u16;
    if (nvs_get_u8(h, "ema", &u8) == ESP_OK) c->ema_pct = u8;
    if (nvs_get_u16(h, "drop", &u16) == ESP_OK) c->dropout_ms = u16;
    if (nvs_get_u8(h, "units", &u8) == ESP_OK) c->units = u8;
    if (nvs_get_u8(h, "full", &u8) == ESP_OK) c->full_every = u8;
    sz = sizeof(c->ssid);
    nvs_get_str(h, "ssid", c->ssid, &sz); /* optional (first boot) */
    nvs_close(h);
    cfg_err_t err;
    if (cfg_validate(c, &err) != 0) {
        ESP_LOGW(TAG, "stored config invalid (%s): reverting to defaults",
                 err.field ? err.field : "?");
        *c = def;
    }
}

int nvs_cfg_store(const cfg_t *c)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) return -1;
    int ok = 0;
    ok |= nvs_set_u16(h, "circ", c->circ_mm);
    ok |= nvs_set_u8(h, "gn", c->gear_num);
    ok |= nvs_set_u8(h, "gd", c->gear_den);
    ok |= nvs_set_u16(h, "deb", c->debounce_ms);
    ok |= nvs_set_u8(h, "ema", c->ema_pct);
    ok |= nvs_set_u16(h, "drop", c->dropout_ms);
    ok |= nvs_set_u8(h, "units", c->units);
    ok |= nvs_set_u8(h, "full", c->full_every);
    if (c->ssid[0]) ok |= nvs_set_str(h, "ssid", c->ssid);
    if (c->pass[0]) ok |= nvs_set_str(h, "pass", c->pass);
    ok |= nvs_commit(h);
    nvs_close(h);
    return ok == 0 ? 0 : -1;
}

int nvs_cfg_wipe(void)
{
    return nvs_flash_erase() == ESP_OK ? 0 : -1;
}
