/* gear-miles main: wires inputs -> domain -> outputs. 1 Hz pump task is
 * the clock for integration, dropout, refresh, and persistence. */
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "board.h"
#include "buttons.h"
#include "gm_api.h"
#include "gm_panel.h"
#include "http_server.h"
#include "storage_nvs.h"
#include "storage_ring.h"
#include "ui.h"
#include "wifi.h"

static const char *TAG = "main";
static api_ctx_t s_ctx;
static rs_t s_ring;
static panel_t s_panel;
const panel_bus_t *panel_bus_hw(void);

static void pump_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

        /* cadence edges -> pipeline */
        int64_t edge_us;
        while (cadence_pop_edge_us(&edge_us)) {
            if (s_ctx.sm.state == ST_RUNNING)
                cad_edge(&s_ctx.cad, (uint32_t)(edge_us / 1000));
        }
        buttons_pump(&s_ctx);
        api_tick(&s_ctx, now);

        /* UI at 1 Hz; panel enforces its own <= 1 Hz budget anyway */
        static uint32_t last_ui = 0;
        if (now - last_ui >= 1000) {
            last_ui = now;
            ui_draw(&s_panel, &s_ctx);
            int rc = panel_refresh_partial(&s_panel);
            s_ctx.display_error = (rc < 0);
            if (rc < 0) ESP_LOGW(TAG, "panel refresh failed (busy timeout %u)",
                                 (unsigned)s_panel.busy_timeouts);
        }
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    board_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_init();

    /* storage: ring + config */
    if (ring_storage_open(&s_ring) != 0)
        ESP_LOGE(TAG, "ring store unavailable; sessions not durable");
    api_init(&s_ctx, &s_ring, (uint32_t)esp_timer_get_time());
    nvs_cfg_load(&s_ctx.cfg);
    s_ctx.cad.cfg.debounce_ms = s_ctx.cfg.debounce_ms;
    s_ctx.cad.cfg.ema_alpha_pct = s_ctx.cfg.ema_pct;
    s_ctx.cad.cfg.dropout_ms = s_ctx.cfg.dropout_ms;

    /* panel */
    if (panel_init(&s_panel, panel_bus_hw(), s_ctx.cfg.full_every) != 0) {
        ESP_LOGE(TAG, "panel init failed — UI error banner only");
        s_ctx.display_error = 1;
    }

    /* network + API (LAN only, no outbound) */
    wifi_stack_init(&s_ctx.cfg);
    wifi_setup_start(&s_ctx.cfg);
    http_server_start(&s_ctx);

    buttons_init();
    ESP_LOGI(TAG, "gear-miles up (host-test contract, no bench evidence yet)");
    xTaskCreate(pump_task, "pump", 4096, NULL, 5, NULL);
}
