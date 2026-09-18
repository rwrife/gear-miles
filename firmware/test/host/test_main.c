/* Host unit tests for gear-miles domain components.
 * Plain C asserts, no framework dependency. Run: make -C firmware/test/host
 * These are HOST/SIMULATION tests — never bench evidence (issue #8). */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gm_api.h"
#include "gm_cadence.h"
#include "gm_config.h"
#include "gm_odometer.h"
#include "gm_panel.h"
#include "gm_ring.h"
#include "gm_session_sm.h"
#include "web_assets.h"

static int g_pass = 0;
#define CHECK(cond) do { if (!(cond)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    return 1; } g_pass++; } while (0)

/* ---------------- cadence ---------------- */

static int test_debounce_window(void)
{
    cad_cfg_t cfg = { .debounce_ms = 30, .ema_alpha_pct = 25, .dropout_ms = 750 };
    cad_t c; cad_init(&c, &cfg);
    CHECK(cad_edge(&c, 1000) == 1);          /* first edge accepted */
    CHECK(cad_edge(&c, 1010) == 0);          /* 10 ms later: bounce, not counted */
    CHECK(cad_edge(&c, 1029) == 0);          /* 29 ms after accepted edge: still bounce */
    CHECK(cad_edge(&c, 1030) == 1);          /* exactly 30 ms: accepted (>= window) */
    CHECK(c.dropped_edges == 2);
    return 0;
}

static int test_ema_convergence(void)
{
    /* Constant 60 RPM stream (1 rev/s -> 60000 ms/gap? no: 60 rev/min = 1 rev/s
     * = 1000 ms gap). EMA should converge to 60 within a few samples. */
    cad_cfg_t cfg = { .debounce_ms = 30, .ema_alpha_pct = 50, .dropout_ms = 5000 };
    cad_t c; cad_init(&c, &cfg);
    uint32_t t = 0;
    int acc = cad_edge(&c, t); CHECK(acc == 1);
    for (int i = 0; i < 10; i++) {
        t += 1000; /* exactly 60 RPM */
        CHECK(cad_edge(&c, t) == 1);
    }
    CHECK(c.rpm == 60); /* converged exactly (alpha=50% on constant input) */

    /* Step from 60 to 120 RPM (500 ms gap): reading must be between the two
     * after one sample and converge up. */
    t += 500; CHECK(cad_edge(&c, t) == 1);
    CHECK(c.rpm >= 60 && c.rpm <= 120);
    for (int i = 0; i < 12; i++) { t += 500; CHECK(cad_edge(&c, t) == 1); }
    CHECK(c.rpm == 120);
    return 0;
}

static int test_dropout_timeout(void)
{
    cad_cfg_t cfg = { .debounce_ms = 30, .ema_alpha_pct = 50, .dropout_ms = 750 };
    cad_t c; cad_init(&c, &cfg);
    CHECK(cad_edge(&c, 0) == 1);
    CHECK(cad_edge(&c, 500) == 1);   /* 120 RPM */
    CHECK(c.rpm > 0);
    cad_tick(&c, 500 + 749);         /* one ms before dropout */
    CHECK(c.rpm > 0);
    cad_tick(&c, 500 + 750);         /* dropout hit: read 0 immediately */
    CHECK(c.rpm == 0);
    /* dropout seconds counting starts at the timeout moment */
    CHECK(cad_dropout_secs(&c, 500 + 750 + 2500) == 2);
    /* first new tick returns to live reading (§6 recovery) */
    CHECK(cad_edge(&c, 500 + 750 + 3000) == 1);
    CHECK(c.rpm > 0);
    return 0;
}

static int test_rpm_clamp(void)
{
    cad_cfg_t cfg = { .debounce_ms = 30, .ema_alpha_pct = 100, .dropout_ms = 5000 };
    cad_t c; cad_init(&c, &cfg);
    CHECK(cad_edge(&c, 0) == 1);
    CHECK(cad_edge(&c, 31) == 1); /* ~1935 RPM raw -> clamped to 300 */
    CHECK(c.rpm == 300);
    return 0;
}

/* ---------------- odometer ---------------- */

static int test_distance_table(void)
{
    /* Hand-computed table. circ = 2100 mm, gear 1:1.
     * 60 RPM = 1 rev/s -> 2100 mm/s = 2.1 m/s.
     * In 100 s: 210000 mm = 210 m. */
    odo_t o; odo_init(&o);
    for (int i = 0; i < 1000; i++) odo_integrate(&o, 60, 100, 2100, 1, 1);
    CHECK(o.dist_mm == 210000u);

    /* 120 RPM for 60 s = 2 rev/s * 2100 mm * 60 s = 252000 mm */
    odo_init(&o);
    for (int i = 0; i < 60; i++) odo_integrate(&o, 120, 1000, 2100, 1, 1);
    CHECK(o.dist_mm == 252000u);

    /* gear 39/13 (3:1): 40 RPM crank -> 120 wheel RPM.
     * 3600 s at 40 RPM: wheel revs = 40/60*3*3600 = 7200; *2100 = 15,120,000 mm */
    odo_init(&o);
    for (int i = 0; i < 3600; i++) odo_integrate(&o, 40, 1000, 2100, 39, 13);
    CHECK(o.dist_mm == 15120000ull);

    /* fractional speed sanity: 60 RPM, 2100 mm = 2.1*3.6 = 7.56 km/h -> 75.6 dkmh
     * rounded = 76 */
    CHECK(odo_speed_dkmh(60, 2100, 1, 1) == 76u);
    /* 100 RPM, 2000 mm: 100/60*2.0 m *3.6 = 12.0 km/h -> 120 */
    CHECK(odo_speed_dkmh(100, 2000, 1, 1) == 120u);
    return 0;
}

static int test_remainder_no_bias(void)
{
    /* A rate that yields <1 mm per step must still integrate exactly over
     * enough time: 1 RPM, 300 mm circ -> 5 mm/min = 0.0833 mm/s.
     * In 6000 s: 30000 mm. Integer steps of 1000 ms alone would give 0. */
    odo_t o; odo_init(&o);
    for (int i = 0; i < 6000; i++) odo_integrate(&o, 1, 1000, 300, 1, 1);
    CHECK(o.dist_mm == 30000u);
    return 0;
}

/* ---------------- ring store ---------------- */

#define RING_CAP (32u + 8u * 32u) /* header slot + 8 records */
typedef struct { uint8_t mem[RING_CAP]; int fail_write_at; } mem_io_t;

static int mem_read(void *u, uint32_t off, uint8_t *b, uint32_t len)
{
    mem_io_t *m = u;
    if (off + len > RING_CAP) return 0;
    memcpy(b, m->mem + off, len);
    return 1;
}
/* Simulate a torn write (power loss): writes to the configured offset only
 * land the first half of their bytes. */
static int mem_write(void *u, uint32_t off, const uint8_t *b, uint32_t len)
{
    mem_io_t *m = u;
    if (off + len > RING_CAP) return 0;
    if (m->fail_write_at == (int)off) {
        memcpy(m->mem + off, b, len / 2);
        return 1;
    }
    memcpy(m->mem + off, b, len);
    return 1;
}

static rs_io_t make_io(mem_io_t *m)
{
    rs_io_t io;
    io.read = mem_read;
    io.write = mem_write;
    io.user = m;
    io.capacity = RING_CAP;
    return io;
}

static rs_rec_t mkrec(uint32_t epoch, uint32_t elapsed, uint64_t dist)
{
    rs_rec_t r = {0};
    r.started_epoch = epoch; r.elapsed_s = elapsed; r.dist_mm = dist;
    r.avg_rpm = 80; r.max_rpm = 120; r.estimate_basis = 1;
    return r;
}

static int test_ring_basic(void)
{
    mem_io_t m = {0};
    /* blank flash (0xFF) triggers format */
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs;
    CHECK(rs_open(&rs, &io) == 0);
    CHECK(rs_slots_free(&rs) == 8);
    for (uint32_t i = 1; i <= 5; i++) {
        rs_rec_t r = mkrec(1000 + i, 60 * i, 1000ull * i);
        CHECK(rs_append(&rs, &r) == 0);
        CHECK(r.seq != i); /* caller's copy untouched */
    }
    CHECK(rs_valid_count(&rs) == 5);
    CHECK(rs_slots_free(&rs) == 3);
    rs_rec_t got;
    CHECK(rs_get(&rs, 3, &got) == 0);
    CHECK(got.seq == 3 && got.elapsed_s == 180 && got.dist_mm == 3000);
    CHECK(rs_get(&rs, 6, &got) == -1); /* not yet written */
    return 0;
}

static int test_ring_wrap(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs;
    CHECK(rs_open(&rs, &io) == 0);
    for (uint32_t i = 0; i < 12; i++) {
        rs_rec_t r = mkrec(i, i, 10ull * i);
        CHECK(rs_append(&rs, &r) == 0);
    }
    /* seq advances monotonically beyond slot count */
    CHECK(rs.seq_next == 13);
    CHECK(rs_slots_free(&rs) == 0);
    rs_rec_t got;
    /* newest 8 survive, oldest 4 evicted */
    CHECK(rs_get(&rs, 12, &got) == 0 && got.elapsed_s == 11);
    CHECK(rs_get(&rs, 5, &got) == 0);
    CHECK(rs_get(&rs, 4, &got) == -1);  /* evicted (age > slot_count) */
    CHECK(rs_get(&rs, 1, &got) == -1);
    return 0;
}

static int test_ring_crc_recovery(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs;
    CHECK(rs_open(&rs, &io) == 0);
    for (uint32_t i = 0; i < 4; i++) {
        rs_rec_t r = mkrec(i, i, 10ull * i);
        CHECK(rs_append(&rs, &r) == 0);
    }
    /* Corrupt the tail record in place (power-loss proxy): flip one byte. */
    uint32_t last_slot_off = 32u + 3u * 32u;
    m.mem[last_slot_off + 10] ^= 0xA5;

    /* Re-open: torn record counted as dropped, not silently overwritten. */
    rs_t rs2;
    CHECK(rs_open(&rs2, &io) == 0);
    CHECK(rs2.dropped == 1);
    CHECK(rs2.slots_valid == 3);
    rs_rec_t got;
    CHECK(rs_get(&rs2, 3, &got) == 0);   /* record 4 (seq 4) is the corrupt one */
    CHECK(rs_get(&rs2, 4, &got) == -1);  /* CRC rejects it on read */
    return 0;
}

static int test_ring_powerloss_torn_tail(void)
{
    /* True "power loss mid-write": writes to the last record only land half
     * the bytes. Reopen must find the ring header intact (separate write),
     * detect the torn tail via CRC, and count it. */
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    int torn_at = (int)(32u + 2u * 32u); /* third record */
    m.fail_write_at = torn_at;
    rs_io_t io = make_io(&m);
    rs_t rs;
    CHECK(rs_open(&rs, &io) == 0);
    rs_rec_t r = mkrec(1, 1, 10);
    CHECK(rs_append(&rs, &r) == 0); /* seq 1 */
    r = mkrec(2, 2, 20);
    CHECK(rs_append(&rs, &r) == 0); /* seq 2 */
    r = mkrec(3, 3, 30);
    CHECK(rs_append(&rs, &r) == 0); /* seq 3: torn */
    rs_t rs2;
    CHECK(rs_open(&rs2, &io) == 0);
    CHECK(rs2.dropped == 1);
    CHECK(rs2.slots_valid == 2);
    return 0;
}

static int test_ring_wipe(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs;
    CHECK(rs_open(&rs, &io) == 0);
    rs_rec_t r = mkrec(1, 1, 10);
    CHECK(rs_append(&rs, &r) == 0);
    CHECK(rs_valid_count(&rs) == 1);
    CHECK(rs_wipe(&rs) == 0);
    CHECK(rs_valid_count(&rs) == 0);
    CHECK(rs.seq_next == 2); /* seq monotonic across wipe */
    CHECK(rs_append(&rs, &r) == 0);
    rs_rec_t got;
    CHECK(rs_get(&rs, 2, &got) == 0);
    /* reopen sees the post-wipe state */
    rs_t rs2;
    CHECK(rs_open(&rs2, &io) == 0);
    CHECK(rs2.slots_valid == 1);
    return 0;
}

/* ---------------- config ---------------- */

static int test_config_validation(void)
{
    cfg_t c; cfg_default(&c);
    cfg_err_t err;
    CHECK(cfg_validate(&c, &err) == 0);
    c.circ_mm = 100;
    CHECK(cfg_validate(&c, &err) == -1);
    CHECK(strcmp(err.field, "circumference_mm") == 0);
    CHECK(strcmp(err.reason, "out_of_range") == 0);
    cfg_default(&c);
    c.gear_num = 0;
    CHECK(cfg_validate(&c, &err) == -1);
    CHECK(strcmp(err.field, "gear_ratio") == 0);
    cfg_default(&c);
    c.debounce_ms = 500;
    CHECK(cfg_validate(&c, &err) == -1);
    CHECK(strcmp(err.field, "debounce_ms") == 0);
    cfg_default(&c);
    c.units = 7;
    CHECK(cfg_validate(&c, &err) == -1);
    CHECK(strcmp(err.field, "units") == 0);
    return 0;
}

/* ---------------- session state machine (arch §5, every row) ---------------- */

static int test_sm_table(void)
{
    sm_t s = { ST_IDLE, 0 };
    uint32_t a;

    /* IDLE */
    a = sm_handle(&s, EV_TAP_A, 100);          CHECK(s.state == ST_RUNNING);
    CHECK(a & SM_ACT_START_SESSION);
    a = sm_handle(&s, EV_TAP_A, 200);          CHECK(s.state == ST_FINISHED);
    a = sm_handle(&s, EV_FINISHED_TIMEOUT, 2300); CHECK(s.state == ST_IDLE);
    CHECK(a & SM_ACT_COMMIT_SESSION);

    sm_t s2 = { ST_IDLE, 0 };
    a = sm_handle(&s2, EV_LONG_B, 10);         CHECK(s2.state == ST_SETTINGS);
    CHECK(a & SM_ACT_ENTER_SETTINGS);
    a = sm_handle(&s2, EV_TAP_B, 20);          CHECK(s2.state == ST_SETTINGS);
    CHECK(a & SM_ACT_NEXT_PAGE);
    a = sm_handle(&s2, EV_TAP_A, 30);          CHECK(s2.state == ST_SETTINGS);
    CHECK(a & SM_ACT_ADJUST_VALUE);
    a = sm_handle(&s2, EV_SETTINGS_TIMEOUT, 40); CHECK(s2.state == ST_IDLE);
    CHECK(a & SM_ACT_PERSIST_CONFIG);

    sm_t s3 = { ST_IDLE, 0 };
    a = sm_handle(&s3, EV_LONG_AB, 0);         CHECK(s3.state == ST_FRC);
    a = sm_handle(&s3, EV_TAP_B, 1);           CHECK(s3.state == ST_IDLE);
    CHECK(a & SM_ACT_ABORT_WIPE);

    sm_t s4 = { ST_IDLE, 0 };
    sm_handle(&s4, EV_LONG_AB, 0);
    a = sm_handle(&s4, EV_TAP_A, 1);           CHECK(s4.state == ST_IDLE);
    CHECK(a & SM_ACT_WIPE);

    sm_t s5 = { ST_IDLE, 0 };
    sm_handle(&s5, EV_TAP_A, 0);               /* RUNNING */
    a = sm_handle(&s5, EV_TAP_B, 1);           CHECK(s5.state == ST_PAUSED);
    CHECK(a & SM_ACT_STOP_TIMER);
    a = sm_handle(&s5, EV_TAP_A, 2);           CHECK(s5.state == ST_RUNNING);
    CHECK(a & SM_ACT_RESUME_TIMER);
    a = sm_handle(&s5, EV_DROPOUT_5MIN, 3);    CHECK(s5.state == ST_PAUSED);
    CHECK(a & SM_ACT_STOP_TIMER);
    a = sm_handle(&s5, EV_TAP_B, 4);           CHECK(s5.state == ST_FINISHED);

    /* longB from RUNNING is not a transition (not in the table) */
    sm_t s6 = { ST_RUNNING, 0 };
    a = sm_handle(&s6, EV_LONG_B, 5);          CHECK(s6.state == ST_RUNNING);
    CHECK(a == 0);
    return 0;
}

/* ---------------- panel HAL (simulated bus) ---------------- */

typedef struct {
    uint8_t dc, cs, rst, busy;
    uint32_t now;
    uint8_t last_cmds[64];
    int n_cmds;
    size_t last_data_len;
    int tx_fail;
} sim_bus_t;

static void sb_dc(void *u, int l)  { ((sim_bus_t*)u)->dc = (uint8_t)l; }
static void sb_cs(void *u, int l)  { ((sim_bus_t*)u)->cs = (uint8_t)l; }
static void sb_rst(void *u, int l) { ((sim_bus_t*)u)->rst = (uint8_t)l; }
static int  sb_busy(void *u)       { return ((sim_bus_t*)u)->busy; }
static int  sb_tx(void *u, const uint8_t *b, size_t n) {
    sim_bus_t *s = u;
    if (s->tx_fail) return -1;
    if (s->dc == 0 && n == 1) { if (s->n_cmds < 64) s->last_cmds[s->n_cmds++] = b[0]; }
    if (s->dc == 1) s->last_data_len = n;
    return 0;
}
static uint32_t sb_now(void *u) { return ((sim_bus_t*)u)->now; }
static void sb_delay(void *u, int ms) { ((sim_bus_t*)u)->now += (uint32_t)ms; }

static int test_panel_sequence(void)
{
    sim_bus_t bus = {0};
    bus.now = 1000;
    panel_bus_t pb = { sb_dc, sb_cs, sb_rst, sb_busy, sb_tx, sb_now, sb_delay, &bus };
    panel_t p;
    CHECK(panel_init(&p, &pb, 30) == 0);
    /* init must contain HW reset, SW reset 0x12, and config commands */
    CHECK(bus.rst == 1);
    int saw12 = 0, saw20 = 0, saw10 = 0;
    for (int i = 0; i < bus.n_cmds; i++) {
        if (bus.last_cmds[i] == 0x12) saw12 = 1;
        if (bus.last_cmds[i] == 0x20) saw20 = 1;
        if (bus.last_cmds[i] == 0x10) saw10 = 1;
    }
    CHECK(saw12 && saw20 && !saw10); /* no deep sleep during init */

    /* full refresh wrote the BW plane (0x24) */
    bus.n_cmds = 0; bus.last_data_len = 0;
    CHECK(panel_refresh_full(&p) == 0);
    int saw24 = 0;
    for (int i = 0; i < bus.n_cmds; i++) if (bus.last_cmds[i] == 0x24) saw24 = 1;
    CHECK(saw24);
    CHECK(p.partials_since_full == 0);

    /* partial refresh budget: immediate retry is deferred (1), after >= 1 s ok */
    bus.now = p.last_refresh_ms + 500;
    CHECK(panel_refresh_partial(&p) == 1);
    bus.now = p.last_refresh_ms + 1000;
    CHECK(panel_refresh_partial(&p) == 0);

    /* after full_every-1 partials it escalates to a full refresh */
    p.partials_since_full = p.full_every - 1;
    bus.now = p.last_refresh_ms + 1000;
    bus.n_cmds = 0;
    CHECK(panel_refresh_partial(&p) == 0);
    int saw26 = 0; /* RED plane = full-refresh marker */
    for (int i = 0; i < bus.n_cmds; i++) if (bus.last_cmds[i] == 0x26) saw26 = 1;
    CHECK(saw26);

    /* deep sleep sets inited=0 and sends 0x10 */
    bus.n_cmds = 0;
    CHECK(panel_deep_sleep(&p) == 0);
    saw10 = 0;
    for (int i = 0; i < bus.n_cmds; i++) if (bus.last_cmds[i] == 0x10) saw10 = 1;
    CHECK(saw10 && !p.inited);

    /* BUSY timeout surfaces as -1 and increments diagnostics */
    bus.busy = 1; /* stuck high forever */
    sim_bus_t b2 = bus; b2.now = 100000; b2.busy = 1;
    panel_bus_t pb2 = pb; pb2.u = &b2;
    panel_t p2;
    /* panel_init polls wait_busy right after 0x12; stuck busy must fail fast */
    CHECK(panel_init(&p2, &pb2, 30) == -1);
    CHECK(p2.busy_timeouts >= 1);
    return 0;
}

static int test_framebuffer(void)
{
    sim_bus_t bus = {0};
    panel_bus_t pb = { sb_dc, sb_cs, sb_rst, sb_busy, sb_tx, sb_now, sb_delay, &bus };
    panel_t p;
    CHECK(panel_init(&p, &pb, 30) == 0);
    panel_fill(&p, 0xFF); /* all white */
    for (int i = 0; i < PANEL_FB_BYTES; i++) CHECK(p.fb[i] == 0xFF);
    panel_draw_str(&p, 0, 0, "1", 0); /* black digit on white */
    int any_black = 0;
    for (int i = 0; i < PANEL_FB_BYTES; i++) if (p.fb[i] != 0xFF) { any_black = 1; break; }
    CHECK(any_black);
    return 0;
}

/* ---------------- API ---------------- */

static int test_api_status_fields(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs; CHECK(rs_open(&rs, &io) == 0);
    api_ctx_t ctx; api_init(&ctx, &rs, 0x1234);

    api_resp_t r;
    api_status(&ctx, &r);
    CHECK(r.status == 200);
    CHECK(strstr(r.body, "\"estimate\":true") != NULL);
    CHECK(strstr(r.body, "estimate_basis") != NULL);
    CHECK(strstr(r.body, "\"state\":\"idle\"") != NULL);
    CHECK(strstr(r.body, "\"history_slots_free\":8") != NULL);

    /* metric-honesty: after a session with distance, exports are labeled */
    api_session_toggle(&ctx, &r); /* start */
    CHECK(strstr(r.body, "running") != NULL);
    ctx.odo.dist_mm = 21430000; /* 21.43 km */
    api_status(&ctx, &r);
    CHECK(strstr(r.body, "\"distance_km\":21.430") != NULL);
    CHECK(strstr(r.body, "\"estimate\":true") != NULL);
    api_session_toggle(&ctx, &r); /* stop -> finished */
    CHECK(strstr(r.body, "finished") != NULL);
    /* finished dwell handled by the periodic pump (2 s) -> commit */
    ctx.uptime_ms = 10000; /* pump base */
    api_tick(&ctx, 12100);
    api_status(&ctx, &r);
    CHECK(strstr(r.body, "\"state\":\"idle\"") != NULL);
    CHECK(strstr(r.body, "\"session_id\":1") != NULL);
    return 0;
}

static int test_api_config_rejects(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs; CHECK(rs_open(&rs, &io) == 0);
    api_ctx_t ctx; api_init(&ctx, &rs, 1);

    api_resp_t r;
    api_config_post(&ctx, "{\"circumference_mm\":100}", &r);
    CHECK(r.status == 400);
    CHECK(strstr(r.body, "\"field\":\"circumference_mm\"") != NULL);
    CHECK(strstr(r.body, "\"reason\":\"out_of_range\"") != NULL);

    /* all-or-nothing: a batch with one bad field leaves config untouched */
    api_config_post(&ctx, "{\"circumference_mm\":2500,\"dropout_ms\":9000}", &r);
    CHECK(r.status == 400);
    CHECK(strstr(r.body, "dropout_ms") != NULL);
    CHECK(ctx.cfg.circ_mm == 2135); /* unchanged */

    /* valid batch applies */
    api_config_post(&ctx, "{\"circumference_mm\":2500,\"units\":1}", &r);
    CHECK(r.status == 200);
    CHECK(ctx.cfg.circ_mm == 2500 && ctx.cfg.units == 1);

    /* unknown keys ignored */
    api_config_post(&ctx, "{\"quantum_mode\":true,\"units\":0}", &r);
    CHECK(r.status == 200 && ctx.cfg.units == 0);

    /* malformed JSON */
    api_config_post(&ctx, "{oops", &r);
    CHECK(r.status == 400);
    CHECK(strstr(r.body, "malformed_json") != NULL);

    /* over-long ssid rejected */
    char big[450];
    strcpy(big, "{\"ssid\":\"");
    for (int i = 0; i < 40; i++) strcat(big, "AAAAAAAAAA");
    strcat(big, "\"}");
    api_config_post(&ctx, big, &r);
    CHECK(r.status == 400);
    CHECK(strstr(r.body, "\"field\":\"ssid\"") != NULL);
    return 0;
}

static int test_api_wipe_nonce(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs; CHECK(rs_open(&rs, &io) == 0);
    api_ctx_t ctx; api_init(&ctx, &rs, 0xBEEF);

    api_resp_t r;
    /* wipe without ever asking for a nonce: rejected */
    api_wipe_post(&ctx, "{\"nonce\":123}", &r);
    CHECK(r.status == 403);

    api_wipe_nonce(&ctx, &r);
    CHECK(r.status == 200);
    char *np = strstr(r.body, "\"nonce\":");
    CHECK(np != NULL);
    uint32_t nonce = (uint32_t)strtoul(np + 8, NULL, 10);
    CHECK(nonce != 0);

    /* wrong nonce rejected */
    api_wipe_post(&ctx, "{\"nonce\":999999}", &r);
    CHECK(r.status == 403);

    /* correct nonce wipes */
    rs_rec_t rec = mkrec(1, 1, 10);
    CHECK(rs_append(&rs, &rec) == 0);
    CHECK(rs_valid_count(&rs) == 1);
    char body[64];
    snprintf(body, sizeof(body), "{\"nonce\":%u}", nonce);
    api_wipe_post(&ctx, body, &r);
    CHECK(r.status == 200);
    CHECK(rs_valid_count(&rs) == 0);

    /* nonce is single-use */
    api_wipe_post(&ctx, body, &r);
    CHECK(r.status == 403);
    return 0;
}

static int test_api_sessions_export(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs; CHECK(rs_open(&rs, &io) == 0);
    api_ctx_t ctx; api_init(&ctx, &rs, 5);

    /* two sessions with distance */
    rs_rec_t a = mkrec(100, 60, 2100000);  /* 2.1 km */
    rs_rec_t b = mkrec(200, 120, 4200000); /* 4.2 km */
    CHECK(rs_append(&rs, &a) == 0);
    CHECK(rs_append(&rs, &b) == 0);

    api_resp_t r;
    api_sessions(&ctx, 10, &r);
    CHECK(r.status == 200);
    CHECK(strstr(r.body, "\"estimate\":true") != NULL);
    CHECK(strstr(r.body, "4.200") != NULL);
    CHECK(strstr(r.body, "2.100") != NULL);

    api_session_detail(&ctx, 2, &r);
    CHECK(r.status == 200);
    CHECK(strstr(r.body, "estimate_basis") != NULL);
    api_session_detail(&ctx, 99, &r);
    CHECK(r.status == 404);

    api_export_csv(&ctx, &r);
    CHECK(r.status == 200);
    CHECK(strstr(r.body, "schema_version=1") != NULL);
    CHECK(strstr(r.body, "distance_km*") != NULL); /* asterisk = estimate legend */
    CHECK(strstr(r.body, "estimate_basis") != NULL);

    /* JSON export (issue #14): sessions-list shape over the full history */
    api_export_json(&ctx, &r);
    CHECK(r.status == 200);
    CHECK(strcmp(r.content_type, "application/json") == 0);
    const char *head = "{\"v\":1,\"schema_version\":1,\"estimate\":true,\"estimate_basis\":";
    CHECK(strncmp(r.body, head, strlen(head)) == 0);
    CHECK(strstr(r.body, "\"estimate_basis\":\"crank cadence x configured circumference\"") != NULL);
    CHECK(strstr(r.body, "\"sessions\":[") != NULL);
    CHECK(strstr(r.body, "\"distance_km\":2.100") != NULL);
    CHECK(strstr(r.body, "\"distance_km\":4.200") != NULL);
    CHECK(strstr(r.body, "],\"truncated\"") == NULL); /* 2 records fit */
    { /* closes as valid JSON: ends exactly `]}` */
        size_t n = strlen(r.body);
        CHECK(n >= 2 && r.body[n - 2] == ']' && r.body[n - 1] == '}');
        /* oldest-first: id 1 appears before id 2 */
        const char *i1 = strstr(r.body, "\"id\":1,");
        const char *i2 = strstr(r.body, "\"id\":2,");
        CHECK(i1 && i2 && i1 < i2);
    }
    return 0;
}

/* JSON export must never emit truncated JSON when the ring outgrows the
 * 1 KiB body: records stop at the budget and the payload gains an explicit
 * "truncated":true sibling (metric honesty — incomplete is visible). */
static int test_api_export_json_truncation(void)
{
    mem_io_t m = {0};
    memset(m.mem, 0xFF, sizeof(m.mem));
    m.fail_write_at = -1;
    rs_io_t io = make_io(&m);
    rs_t rs; CHECK(rs_open(&rs, &io) == 0);
    CHECK(rs.slot_count == 8);
    api_ctx_t ctx; api_init(&ctx, &rs, 7);

    for (uint32_t i = 0; i < 8; i++) {
        rs_rec_t rec = mkrec(1758000000u + i * 86400u, 3600u + i, 4294967000ull);
        CHECK(rs_append(&rs, &rec) == 0);
    }
    api_resp_t r;
    api_export_json(&ctx, &r);
    CHECK(r.status == 200);
    CHECK(strstr(r.body, "\"truncated\":true}") != NULL);
    { /* closes as valid JSON even at the budget edge: `...],"truncated":true}` */
        size_t n = strlen(r.body);
        const char *tail = "],\"truncated\":true}";
        size_t tl = strlen(tail);
        CHECK(n > tl && strcmp(r.body + n - tl, tail) == 0);
    }
    /* every emitted record is whole: count of '{' equals count of '}' in the
     * sessions array region — snprintf truncation can't split a record */
    {
        int opens = 0, closes = 0;
        for (const char *q = r.body; *q; q++) {
            if (*q == '{') opens++;
            if (*q == '}') closes++;
        }
        CHECK(opens == closes);
        CHECK(opens >= 2 && opens <= 8); /* envelope + >=1 but not all 8 records */
    }
    CHECK(strstr(r.body, "\"id\":8,") == NULL);
    return 0;
}

/* ---------------- runner ---------------- */

/* Embedded dashboard bundle (issue #7): the committed generated table must
 * contain an index route and every referenced asset path. This links the
 * generated web_assets.c so CI proves the embed compiles and resolves. */
static int test_web_assets_lookup(void)
{
    CHECK(gm_web_assets_count >= 3);
    const gm_web_asset_t *root = gm_web_find("/");
    CHECK(root != NULL);
    CHECK(root && strstr(root->mime, "text/html") != NULL);
    CHECK(root && root->len > 100);
    /* every asset referenced from index.html must resolve */
    CHECK(root != NULL);
    if (root) {
        char html[8192];
        size_t n = root->len < sizeof(html) - 1 ? root->len : sizeof(html) - 1;
        memcpy(html, root->data, n);
        html[n] = 0;
        const char *p = html;
        int refs = 0;
        while ((p = strstr(p, "./assets/")) != NULL) {
            char path[128];
            size_t i = 0;
            const char *q = p + 1; /* skip leading '.', keep /assets/... */
            while (*q && *q != '"' && i < sizeof(path) - 1) path[i++] = *q++;
            path[i] = 0;
            CHECK(gm_web_find(path) != NULL);
            refs++;
            p = q;
        }
        CHECK(refs >= 2); /* css + js from the real vite build */
    }
    CHECK(gm_web_find("/../../etc/passwd") == NULL);
    CHECK(gm_web_find("/nope") == NULL);
    return 0;
}

typedef int (*test_fn)(void);
struct test { const char *name; test_fn fn; };
static struct test tests[] = {
    {"debounce_window", test_debounce_window},
    {"ema_convergence", test_ema_convergence},
    {"dropout_timeout", test_dropout_timeout},
    {"rpm_clamp", test_rpm_clamp},
    {"distance_table", test_distance_table},
    {"remainder_no_bias", test_remainder_no_bias},
    {"ring_basic", test_ring_basic},
    {"ring_wrap", test_ring_wrap},
    {"ring_crc_recovery", test_ring_crc_recovery},
    {"ring_powerloss_torn_tail", test_ring_powerloss_torn_tail},
    {"ring_wipe", test_ring_wipe},
    {"config_validation", test_config_validation},
    {"sm_table", test_sm_table},
    {"panel_sequence", test_panel_sequence},
    {"framebuffer", test_framebuffer},
    {"api_status_fields", test_api_status_fields},
    {"api_config_rejects", test_api_config_rejects},
    {"api_wipe_nonce", test_api_wipe_nonce},
    {"api_sessions_export", test_api_sessions_export},
    {"api_export_json_truncation", test_api_export_json_truncation},
    {"web_assets_lookup", test_web_assets_lookup},
};

int main(void)
{
    int failed = 0;
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        g_pass = 0;
        int rc = tests[i].fn();
        printf("%-28s %s (%d checks)\n", tests[i].name, rc ? "FAIL" : "PASS", g_pass);
        failed += rc;
    }
    printf("%zu tests, %d failures\n", sizeof(tests)/sizeof(tests[0]), failed);
    return failed ? 1 : 0;
}
