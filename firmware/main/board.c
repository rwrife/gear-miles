#include "board.h"
#include "driver/gpio.h"

void board_init(void)
{
    /* Outputs: panel control lines, default inactive. */
    gpio_set_direction(PIN_INK_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_INK_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_INK_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_INK_CS, 1);
    gpio_set_level(PIN_INK_RST, 1);

    /* Inputs with pull-ups: reed sensor (external 100k present as well),
     * buttons to GND. BOOT stays input-pull-up only. */
    gpio_set_direction(PIN_CADENCE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_CADENCE, GPIO_PULLUP_ONLY);
    gpio_set_direction(PIN_BTN_A, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_A, GPIO_PULLUP_ONLY);
    gpio_set_direction(PIN_BTN_B, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BTN_B, GPIO_PULLUP_ONLY);
    gpio_set_direction(PIN_BOOT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BOOT, GPIO_PULLUP_ONLY);
}
