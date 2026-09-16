/* Gear Miles — e-ink panel HAL + GDEY029T94 driver (pure C).
 *
 * The driver is isolated behind an injected bus so host/simulation tests
 * exist without hardware (issue #6 acceptance). Command sequence follows
 * GDEY029T94 datasheet (Rev 1.0, 2021-03-15) §14.1 "Normal Operation
 * Flow" — datasheet in datasheets/GDEY029T94.pdf, recorded with SHA-256
 * in docs/datasheet-sources.md.
 *
 * HONESTY: the sequence is transcribed from the datasheet text and is
 * NOT verified against silicon. Bench validation is issue #8 scope.
 * BS1 = L selects 4-line SPI (DS Note 5-5); the board straps BS1 low.
 */
#ifndef GM_PANEL_H
#define GM_PANEL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Panel framebuffer: 296 x 128 px, 1 bpp (DS §3 resolution), 4736 bytes. */
#define PANEL_W 296
#define PANEL_H 128
#define PANEL_FB_BYTES (PANEL_W * PANEL_H / 8)

/* Refresh policy, per GDEY029T94 DS: full 3 s, fast 1.5 s, partial 0.3 s
 * (§6.3/§7 update timings); architecture §6 requires partial <= 1 Hz with
 * a forced full refresh every N partials (default 30, tuned at #8). */
#define PANEL_MIN_PARTIAL_INTERVAL_MS 1000u
#define PANEL_DEFAULT_FULL_EVERY 30u

typedef struct panel_bus {
    void (*set_dc)(void *u, int level);
    void (*set_cs)(void *u, int level);
    void (*set_rst)(void *u, int level);
    int  (*busy_level)(void *u);            /* 1 = BUSY (DS Note 5-4) */
    int  (*tx)(void *u, const uint8_t *b, size_t len); /* SPI write */
    uint32_t (*now_ms)(void *u);
    void (*delay_ms)(void *u, int ms);
    void *u;
} panel_bus_t;

typedef struct {
    const panel_bus_t *bus;
    uint8_t fb[PANEL_FB_BYTES];     /* current image */
    uint8_t inited;                 /* init/RESET-complete flag */
    uint32_t last_refresh_ms;       /* 0 = never */
    uint32_t partials_since_full;
    uint32_t busy_timeouts;         /* diagnostics (driver reports errors) */
    uint8_t  full_every;
} panel_t;

/* Power-on sequence: HW reset pulse + SW reset 0x12 + configuration
 * (0x01 gate, 0x11 data-entry, 0x44/0x45 RAM x, 0x3C border) + LUT from
 * OTP and temperature sensor selection (0x18). Returns 0, -1 on BUSY
 * timeout. */
int panel_init(panel_t *p, const panel_bus_t *bus, uint8_t full_every);

/* Full-screen refresh (writes both B/W RAM planes 0x24 + 0x26 then
 * 0x22/0x20 activate per DS §14.1 step 5). */
int panel_refresh_full(panel_t *p);

/* Partial refresh of the changed region (0x24 then 0x22/0x20). Enforces
 * the <= 1 Hz budget: returns 1 (= deferred, caller keeps old frame) if
 * now-last < PANEL_MIN_PARTIAL_INTERVAL_MS. After `full_every` partials
 * it silently escalates to a full refresh (ghosting policy, §6). */
int panel_refresh_partial(panel_t *p);

/* Deep sleep (0x10, "1 -> soft and hard reset are enabled before PSRR
 * (LDO will be OFF, only digital pads are VDD-levied)", DS §7 0x10):
 * called when a session ends so the panel draws 1-5 uA (DS §6.2). */
int panel_deep_sleep(panel_t *p);

/* Framebuffer helpers. */
void panel_fill(panel_t *p, uint8_t pattern);
void panel_draw_char(panel_t *p, int x, int y, char ch, int invert);
void panel_draw_str(panel_t *p, int x, int y, const char *s, int invert);

#ifdef __cplusplus
}
#endif
#endif /* GM_PANEL_H */
