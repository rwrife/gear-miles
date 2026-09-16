/* Board pin map — frozen contract from docs/architecture.md §3.
 * Binding: ESP32-C3-WROOM-02 datasheet v1.7 Table 3-1 (see
 * docs/component-selection.md C1). Any change here requires an
 * architecture PR. */
#ifndef GM_BOARD_H
#define GM_BOARD_H

#include "driver/gpio.h"

#define PIN_INK_CLK   GPIO_NUM_6
#define PIN_INK_MOSI  GPIO_NUM_7
#define PIN_INK_CS    GPIO_NUM_10
#define PIN_INK_DC    GPIO_NUM_3
#define PIN_INK_RST   GPIO_NUM_4
#define PIN_INK_BUSY  GPIO_NUM_5
#define PIN_CADENCE   GPIO_NUM_1   /* external 100k PU (C15), reed to GND */
#define PIN_BTN_A     GPIO_NUM_0
#define PIN_BTN_B     GPIO_NUM_8
#define PIN_BOOT      GPIO_NUM_9   /* download-boot button/TP */
#define PIN_U0_RXD    GPIO_NUM_20
#define PIN_U0_TXD    GPIO_NUM_21
/* GPIO2 left unconnected on the PCB (strapping, arch §3). */

/* Session ring lives on the 'storage' raw data partition (partitions.csv). */
#define RING_PARTITION "storage"

void board_init(void);

#endif
