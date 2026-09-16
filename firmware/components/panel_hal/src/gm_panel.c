#include <string.h>
#include "gm_panel.h"

/* GDEY029T94 command set (DS §7 Command Table, Rev 1.0 2021-03-15). */
#define CMD_DRIVER_OUT   0x01
#define CMD_DATA_ENTRY   0x11
#define CMD_SW_RESET     0x12
#define CMD_TEMP_SENSE   0x18
#define CMD_LUT_OTP      0x22
#define CMD_MASTER       0x20
#define CMD_WRITE_RAM_BW 0x24
#define CMD_WRITE_RAM_RED 0x26
#define CMD_RAM_X_LEFT   0x44
#define CMD_RAM_X_RIGHT  0x45
#define CMD_RAM_Y_START  0x4E
#define CMD_RAM_Y_END    0x4F
#define CMD_BORDER       0x3C
#define CMD_SOFT_START   0x0C
#define CMD_DEEP_SLEEP   0x10

#define BUSY_POLL_MAX_MS 5000 /* full refresh typ 3 s (DS §6.3) + margin */

static int cmd(panel_t *p, uint8_t c)
{
    uint8_t b = c;
    p->bus->set_cs(p->bus->u, 0);
    p->bus->set_dc(p->bus->u, 0); /* D/C# low = command */
    int r = p->bus->tx(p->bus->u, &b, 1);
    p->bus->set_cs(p->bus->u, 1);
    return r;
}

static int data(panel_t *p, const uint8_t *b, size_t n)
{
    p->bus->set_cs(p->bus->u, 0);
    p->bus->set_dc(p->bus->u, 1); /* D/C# high = data */
    int r = p->bus->tx(p->bus->u, b, n);
    p->bus->set_cs(p->bus->u, 1);
    return r;
}

/* BUSY polarity per DS Note 5-4: busy state output; driver polls via the
 * injected busy_level() which normalizes to 1 = busy. */
static int wait_busy(panel_t *p, uint32_t max_ms)
{
    uint32_t t0 = p->bus->now_ms(p->bus->u);
    while (p->bus->busy_level(p->bus->u)) {
        if (p->bus->now_ms(p->bus->u) - t0 > max_ms) {
            p->busy_timeouts++;
            return -1;
        }
        p->bus->delay_ms(p->bus->u, 1);
    }
    return 0;
}

int panel_init(panel_t *p, const panel_bus_t *bus, uint8_t full_every)
{
    memset(p, 0, sizeof(*p));
    p->bus = bus;
    p->full_every = full_every ? full_every : PANEL_DEFAULT_FULL_EVERY;
    panel_fill(p, 0xFF); /* white */

    /* DS §14.1 step 1-2: power on, wait 10 ms, HW reset, SW reset. */
    bus->delay_ms(bus->u, 10);
    bus->set_rst(bus->u, 0);
    bus->delay_ms(bus->u, 10);
    bus->set_rst(bus->u, 1);
    bus->delay_ms(bus->u, 10);
    if (cmd(p, CMD_SW_RESET) < 0) return -1;
    if (wait_busy(p, 1000) < 0) return -1;

    /* Step 3: initial configuration — gate driver, entry mode, RAM size,
     * border. Values from DS §14.1 command list; gate settings follow the
     * SSD16xx-class 0x01 triple (A[0]=OD, CL=0, CLSEL=2) used in Good
     * Display reference code shipped with the panel family. */
    uint8_t g[3] = {0x03, 0x00, 0x2B};
    if (cmd(p, CMD_DRIVER_OUT) < 0) return -1;
    if (data(p, g, 3) < 0) return -1;
    uint8_t de = 0x03; /* X increment, Y increment (DS 0x11) */
    if (cmd(p, CMD_DATA_ENTRY) < 0) return -1;
    if (data(p, &de, 1) < 0) return -1;
    /* RAM x-range: 24-bit address units, 296 px = 37 bytes per row.
     * Left 0x0000, right 0x0127 (37*8-1 = 295). */
    uint8_t xl[2] = {0x00, 0x00}, xr[2] = {0x27, 0x01};
    if (cmd(p, CMD_RAM_X_LEFT) < 0) return -1;
    if (data(p, xl, 2) < 0) return -1;
    if (cmd(p, CMD_RAM_X_RIGHT) < 0) return -1;
    if (data(p, xr, 2) < 0) return -1;
    /* border waveform: VBDF=1 (0x3C, DS §14.1 "set panel border") */
    uint8_t bd = 0xB7;
    if (cmd(p, CMD_BORDER) < 0) return -1;
    if (data(p, &bd, 1) < 0) return -1;

    /* Step 4: temperature source + LUT from OTP. */
    uint8_t ts = 0x80; /* internal sensor (DS 0x18) */
    if (cmd(p, CMD_TEMP_SENSE) < 0) return -1;
    if (data(p, &ts, 1) < 0) return -1;
    uint8_t lut = 0xB0; /* LOAD LUT FROM OTP (DS 0x22) */
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    if (data(p, &lut, 1) < 0) return -1;
    if (cmd(p, CMD_MASTER) < 0) return -1;
    if (wait_busy(p, BUSY_POLL_MAX_MS) < 0) return -1;

    p->inited = 1;
    return 0;
}

int panel_refresh_full(panel_t *p)
{
    if (!p->inited) return -1;
    /* DS §14.1 step 5: write both planes, set soft start, activate. */
    if (cmd(p, CMD_SOFT_START) < 0) return -1;
    uint8_t ss[3] = {0xAE, 0xC7, 0xC3}; /* reference soft-start setting */
    if (data(p, ss, 3) < 0) return -1;

    uint8_t y0[2] = {0x00, 0x00};
    if (cmd(p, CMD_RAM_Y_START) < 0) return -1;
    if (data(p, y0, 2) < 0) return -1;
    if (cmd(p, CMD_WRITE_RAM_BW) < 0) return -1;
    if (data(p, p->fb, PANEL_FB_BYTES) < 0) return -1;
    if (cmd(p, CMD_RAM_Y_END) < 0) return -1;
    if (data(p, y0, 2) < 0) return -1;
    if (cmd(p, CMD_WRITE_RAM_RED) < 0) return -1;
    { /* RED plane all-white = full update (128 rows of 0xFF) */
        uint8_t whites[37];
        memset(whites, 0xFF, sizeof(whites));
        for (int i = 0; i < PANEL_H; i++)
            if (data(p, whites, sizeof(whites)) < 0) return -1;
    }
    uint8_t a = 0xC6; /* DISABLE_ANALOG (DS 0x22, after update) */
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    if (data(p, &a, 1) < 0) return -1;
    /* C6 alone does not drive; reference code loads then activates.
     * DS §14.1: "Drive display panel by Command 0x22, 0x20". */
    a = 0x03; /* ENABLE ANALOG + CLOCK + DISPLAY (DS 0x22) */
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    if (data(p, &a, 1) < 0) return -1;
    if (cmd(p, CMD_MASTER) < 0) return -1;
    if (wait_busy(p, BUSY_POLL_MAX_MS) < 0) return -1;
    uint8_t off = 0xC6;
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    if (data(p, &off, 1) < 0) return -1;

    p->last_refresh_ms = p->bus->now_ms(p->bus->u);
    p->partials_since_full = 0;
    return 0;
}

int panel_refresh_partial(panel_t *p)
{
    if (!p->inited) return -1;
    uint32_t now = p->bus->now_ms(p->bus->u);
    if (p->last_refresh_ms != 0 &&
        now - p->last_refresh_ms < PANEL_MIN_PARTIAL_INTERVAL_MS) {
        return 1; /* deferred: budget is <= 1 partial refresh per second */
    }
    if (p->partials_since_full + 1u >= p->full_every) {
        /* ghosting policy: escalate to a full refresh (architecture §6) */
        return panel_refresh_full(p);
    }
    /* Partial: single BW plane update with OTP LUT (DS §14.1 step 5,
     * partial variant omits the RED plane). */
    uint8_t y0[2] = {0x00, 0x00};
    uint8_t a;
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    uint8_t lut = 0xB1; /* LUT option for partial (ref code 0x22=B1) */
    if (data(p, &lut, 1) < 0) return -1;
    if (cmd(p, CMD_RAM_Y_START) < 0) return -1;
    if (data(p, y0, 2) < 0) return -1;
    if (cmd(p, CMD_WRITE_RAM_BW) < 0) return -1;
    if (data(p, p->fb, PANEL_FB_BYTES) < 0) return -1;
    a = 0x0F; /* ANALOG + CLOCK + TEMP + DISPLAY (partial activate) */
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    if (data(p, &a, 1) < 0) return -1;
    if (cmd(p, CMD_MASTER) < 0) return -1;
    if (wait_busy(p, BUSY_POLL_MAX_MS) < 0) return -1;
    a = 0xC6;
    if (cmd(p, CMD_LUT_OTP) < 0) return -1;
    if (data(p, &a, 1) < 0) return -1;
    (void)y0;

    p->last_refresh_ms = now;
    p->partials_since_full++;
    return 0;
}

int panel_deep_sleep(panel_t *p)
{
    if (!p->inited) return -1;
    uint8_t d = 0x01; /* "1 -> soft and hard reset enabled before PSRR,
                       * LDO off" (DS §7, 0x10 row) */
    if (cmd(p, CMD_DEEP_SLEEP) < 0) return -1;
    if (data(p, &d, 1) < 0) return -1;
    p->inited = 0; /* wake requires the §14.1 init flow again */
    return 0;
}

void panel_fill(panel_t *p, uint8_t pattern)
{
    memset(p->fb, pattern, PANEL_FB_BYTES);
}

/* Minimal 5x7 ASCII font for digits/space/colon/dash and the letters the
 * UI needs; other codepoints render as boxes. Keeping glyphs in-tree makes
 * the UI testable without lvgl/font tooling. */
struct glyph { uint8_t rows[7]; };
static const struct glyph *glyph_for(char ch);

static void draw_glyph(panel_t *p, int x, int y, const struct glyph *g, int invert)
{
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 5; c++) {
            int px = x + c, py = y + r;
            if (px < 0 || py < 0 || px >= PANEL_W || py >= PANEL_H) continue;
            int on = (g->rows[r] >> (4 - c)) & 1;
            int bit = (invert ? !on : on);
            int byte = py * (PANEL_W / 8) + px / 8;
            uint8_t mask = (uint8_t)(0x80 >> (px % 8));
            if (bit) p->fb[byte] &= (uint8_t)~mask; /* 0 = black pixel */
            else     p->fb[byte] |= mask;
        }
}

void panel_draw_char(panel_t *p, int x, int y, char ch, int invert)
{
    const struct glyph *g = glyph_for(ch);
    draw_glyph(p, x, y, g, invert);
}

void panel_draw_str(panel_t *p, int x, int y, const char *s, int invert)
{
    while (*s) { panel_draw_char(p, x, y, *s++, invert); x += 6; }
}

/* ---- 5x7 font (subset) ---- */
#define G(...) {{ __VA_ARGS__ }}
static const struct glyph font['9' - '0' + 1] = {
    G(0x0E,0x11,0x13,0x15,0x19,0x11,0x0E),   /* 0 */
    G(0x04,0x0C,0x04,0x04,0x04,0x04,0x0E),   /* 1 */
    G(0x0E,0x11,0x01,0x02,0x04,0x08,0x1F),   /* 2 */
    G(0x0E,0x11,0x01,0x06,0x01,0x11,0x0E),   /* 3 */
    G(0x02,0x06,0x0A,0x12,0x1F,0x02,0x02),   /* 4 */
    G(0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E),   /* 5 */
    G(0x06,0x08,0x10,0x1E,0x11,0x11,0x0E),   /* 6 */
    G(0x1F,0x01,0x02,0x04,0x08,0x08,0x08),   /* 7 */
    G(0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E),   /* 8 */
    G(0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C),   /* 9 */
};
static const struct glyph gl_space = G(0,0,0,0,0,0,0);
static const struct glyph gl_dot   = G(0,0,0,0,0x0C,0x0C,0);
static const struct glyph gl_colon = G(0,0x0C,0x0C,0,0x0C,0x0C,0);
static const struct glyph gl_dash  = G(0,0,0,0x1F,0,0,0);
static const struct glyph gl_star  = G(0x04,0x15,0x0E,0x1F,0x0E,0x15,0x04);
static const struct glyph gl_q     = G(0x0E,0x11,0x13,0x15,0x1B,0x10,0x0D);
static const struct glyph gl_box   = G(0x1F,0x11,0x11,0x11,0x11,0x11,0x1F);
static const struct glyph gl_amark = G(0x0E,0x11,0x11,0x1F,0x11,0x11,0x11); /* A */
static const struct glyph gl_k     = G(0x11,0x12,0x14,0x18,0x14,0x12,0x11); /* K */
static const struct glyph gl_m     = G(0x11,0x1B,0x15,0x15,0x11,0x11,0x11); /* M */
static const struct glyph gl_p     = G(0x1E,0x11,0x11,0x1E,0x10,0x10,0x10); /* P */
static const struct glyph gl_s     = G(0x07,0x08,0x10,0x0E,0x01,0x01,0x1E); /* S */
static const struct glyph gl_t     = G(0x1F,0x04,0x04,0x04,0x04,0x04,0x04); /* T */
static const struct glyph gl_h     = G(0x11,0x11,0x11,0x1F,0x11,0x11,0x11); /* H */
static const struct glyph gl_i     = G(0x0E,0x04,0x04,0x04,0x04,0x04,0x0E); /* I */
static const struct glyph gl_l     = G(0x10,0x10,0x10,0x10,0x10,0x10,0x1F); /* L */
static const struct glyph gl_e     = G(0x0E,0x11,0x10,0x1F,0x10,0x11,0x0E); /* E */
static const struct glyph gl_r     = G(0x1E,0x11,0x11,0x1E,0x14,0x12,0x11); /* R */
static const struct glyph gl_d     = G(0x1C,0x12,0x11,0x11,0x11,0x12,0x1C); /* D */
static const struct glyph gl_n     = G(0x11,0x19,0x15,0x13,0x11,0x11,0x11); /* N */
static const struct glyph gl_u     = G(0x11,0x11,0x11,0x11,0x11,0x11,0x0F); /* U */
static const struct glyph gl_w     = G(0x11,0x11,0x15,0x15,0x1B,0x11,0x11); /* W */
static const struct glyph gl_f     = G(0x1F,0x10,0x10,0x1E,0x10,0x10,0x10); /* F */

static const struct glyph *glyph_for(char ch)
{
    if (ch >= '0' && ch <= '9') return &font[ch - '0'];
    switch (ch) {
    case ' ': return &gl_space;
    case '.': return &gl_dot;
    case ':': return &gl_colon;
    case '-': case '_': return &gl_dash;
    case '*': return &gl_star;
    case '?': return &gl_q;
    case 'A': return &gl_amark;
    case 'K': return &gl_k;
    case 'M': return &gl_m;
    case 'P': return &gl_p;
    case 'S': return &gl_s;
    case 'T': return &gl_t;
    case 'H': return &gl_h;
    case 'I': return &gl_i;
    case 'L': return &gl_l;
    case 'E': return &gl_e;
    case 'R': return &gl_r;
    case 'D': return &gl_d;
    case 'N': return &gl_n;
    case 'U': return &gl_u;
    case 'W': return &gl_w;
    case 'F': return &gl_f;
    default:  return &gl_box;
    }
}
#undef G
