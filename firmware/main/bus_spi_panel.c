/* SPI bus adapter: maps panel_bus_t onto esp_driver_spi (host tests inject
 * a fake bus instead). Panel wiring per board.h; BS1 is strapped LOW on the
 * board (4-line SPI, GDEY029T94 DS Note 5-5). */
#include <string.h>
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "board.h"
#include "gm_panel.h"

#define SPI_HOST SPI2_HOST
#define SPI_HZ   2000000 /* panel max SPI clk ~20 MHz (DS §6.3); 2 MHz safe */

static spi_device_handle_t s_dev;
static int s_dc_level;

static void bus_set_dc(void *u, int level)
{
    (void)u;
    s_dc_level = level;
    gpio_set_level(PIN_INK_DC, level);
}
static void bus_set_cs(void *u, int level)
{
    (void)u;
    gpio_set_level(PIN_INK_CS, level);
}
static void bus_set_rst(void *u, int level)
{
    (void)u;
    gpio_set_level(PIN_INK_RST, level);
}
static int bus_busy(void *u)
{
    (void)u;
    /* DS Note 5-4: BUSY is active-high while the panel drives. */
    return gpio_get_level(PIN_INK_BUSY) == 1;
}
static int bus_tx(void *u, const uint8_t *b, size_t len)
{
    (void)u;
    /* SPI transaction; D/C set via GPIO alongside (see bus_set_dc). */
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = b,
        .user = (void *)(intptr_t)s_dc_level,
    };
    esp_err_t err = spi_device_polling_transmit(s_dev, &t);
    return err == ESP_OK ? 0 : -1;
}
static uint32_t bus_now(void *u)
{
    (void)u;
    return (uint32_t)(esp_timer_get_time() / 1000);
}
static void bus_delay(void *u, int ms)
{
    (void)u;
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static const panel_bus_t s_bus = {
    .set_dc = bus_set_dc, .set_cs = bus_set_cs, .set_rst = bus_set_rst,
    .busy_level = bus_busy, .tx = bus_tx, .now_ms = bus_now,
    .delay_ms = bus_delay, .u = NULL,
};

const panel_bus_t *panel_bus_hw(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_INK_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_INK_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PANEL_FB_BYTES + 8,
    };
    if (spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK)
        abort();
    spi_device_interface_config_t dcfg = {
        .clock_speed_hz = SPI_HZ,
        .mode = 0, /* CPOL=0 CPHA=0 (DS §6.3.2 timing) */
        .spics_io_num = -1, /* CS handled explicitly via GPIO */
        .queue_size = 1,
    };
    if (spi_bus_add_device(SPI_HOST, &dcfg, &s_dev) != ESP_OK)
        abort();
    return &s_bus;
}
