/* Button + cadence input. GPIO ISR timestamps edges into queues; the main
 * task pumps them. Buttons are gesture-classified here (hardware-side
 * 30 ms bounce filter; the cadence pipeline debounces in gm_cadence). */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "board.h"
#include "gm_api.h"
#include "gm_session_sm.h"
#include "buttons.h"

typedef struct {
    int pin;
    int64_t ts_us;
} edge_msg_t;

static QueueHandle_t s_btn_q;
static QueueHandle_t s_cad_q;

typedef struct {
    int64_t last_edge_us;
    int64_t press_start_us;
    int held;
    int long_fired; /* long gesture consumed; swallow the release */
} btn_state_t;

static btn_state_t s_a, s_b;

void buttons_pump(api_ctx_t *ctx)
{
    edge_msg_t e;
    while (xQueueReceive(s_btn_q, &e, 0) == pdTRUE) {
        btn_state_t *st = (e.pin == PIN_BTN_A) ? &s_a
                          : (e.pin == PIN_BTN_B) ? &s_b : NULL;
        if (!st) continue;
        /* hardware bounce filter: ignore edges < 30 ms apart */
        if (e.ts_us - st->last_edge_us < 30000) continue;
        st->last_edge_us = e.ts_us;
        int level = gpio_get_level(e.pin);
        if (level == 0 && !st->held) {
            st->held = 1;
            st->press_start_us = e.ts_us;
        } else if (level == 1 && st->held) {
            st->held = 0;
            if (st->long_fired) { st->long_fired = 0; continue; }
            int64_t dur = e.ts_us - st->press_start_us;
            if (e.pin == PIN_BTN_B && dur >= 3000000) { /* BTN_B >= 3 s: longB */
                sm_handle(&ctx->sm, EV_LONG_B, ctx->uptime_ms);
            } else if (dur < 3000000) {
                sm_handle(&ctx->sm,
                          e.pin == PIN_BTN_A ? EV_TAP_A : EV_TAP_B,
                          ctx->uptime_ms);
            }
        }
    }
    /* longAB: both held >= 5 s (arch §5), checked on the pump cadence */
    int64_t now = esp_timer_get_time();
    if (s_a.held && s_b.held && !s_a.long_fired) {
        int64_t both_since = (s_a.press_start_us > s_b.press_start_us)
                             ? s_a.press_start_us : s_b.press_start_us;
        if (now - both_since >= 5000000) {
            sm_handle(&ctx->sm, EV_LONG_AB, ctx->uptime_ms);
            s_a.long_fired = s_b.long_fired = 1;
        }
    }
}

int cadence_pop_edge_us(int64_t *out_us)
{
    edge_msg_t e;
    if (xQueueReceive(s_cad_q, &e, 0) == pdTRUE) { *out_us = e.ts_us; return 1; }
    return 0;
}

static void IRAM_ATTR isr_edge(void *arg)
{
    edge_msg_t e = { .pin = (int)(intptr_t)arg, .ts_us = esp_timer_get_time() };
    BaseType_t hi = pdFALSE;
    if (e.pin == PIN_CADENCE)
        xQueueSendFromISR(s_cad_q, &e, &hi);
    else
        xQueueSendFromISR(s_btn_q, &e, &hi);
    if (hi) portYIELD_FROM_ISR();
}

void buttons_init(void)
{
    s_btn_q = xQueueCreate(16, sizeof(edge_msg_t));
    s_cad_q = xQueueCreate(64, sizeof(edge_msg_t));
    gpio_config_t io = {
        .pin_bit_mask = (1ull << PIN_BTN_A) | (1ull << PIN_BTN_B) |
                        (1ull << PIN_CADENCE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BTN_A, isr_edge, (void *)(intptr_t)PIN_BTN_A);
    gpio_isr_handler_add(PIN_BTN_B, isr_edge, (void *)(intptr_t)PIN_BTN_B);
    gpio_isr_handler_add(PIN_CADENCE, isr_edge, (void *)(intptr_t)PIN_CADENCE);
}
