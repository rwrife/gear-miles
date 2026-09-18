/* LAN API contract stub — HOST/SIMULATION tool, never bench evidence.
 *
 * Serves the *actual* firmware domain API handlers (components/domain
 * gm_api.c et al.) over HTTP/1.1 on 127.0.0.1 so the dashboard's contract
 * tests (app/tests/contract) run against real firmware response code, not a
 * re-implementation. Route table mirrors main/http_server.c.
 *
 * Optional static hosting (for the accessibility smoke test): set
 * GM_STUB_DIST=<dir> to serve an app build (index.html + assets) exactly
 * one origin above the API, like the device does. This static path exists
 * ONLY for the smoke harness; production embedding of assets in the flash
 * image is a separate firmware step (tracked, see app/README.md).
 *
 * Seeded fixtures via env:
 *   GM_STUB_PORT   listen port              (default 8123)
 *   GM_STUB_SEED   xorshift boot seed       (default 42)
 *   GM_STUB_SEEDS  committed sessions to pre-seed (default 3)
 *   GM_STUB_DIST   static asset dir         (default: none -> placeholder root)
 */
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gm_api.h"

/* ---------------- RAM-backed ring io (mirrors test/host pattern) -------- */
#define MEM_CAP 16384
typedef struct {
    uint8_t mem[MEM_CAP];
} mem_io_t;

static int mem_read(void *u, uint32_t off, uint8_t *buf, uint32_t len)
{
    mem_io_t *m = u;
    if (off + len > MEM_CAP) return 0;
    memcpy(buf, m->mem + off, len);
    return 1; /* ring io convention: 1 = ok (see rs_open) */
}
static int mem_write(void *u, uint32_t off, const uint8_t *buf, uint32_t len)
{
    mem_io_t *m = u;
    if (off + len > MEM_CAP) return 0;
    memcpy(m->mem + off, buf, len);
    return 1;
}
static int mem_erase(void *u, uint32_t off, uint32_t len)
{
    mem_io_t *m = u;
    if (off + len > MEM_CAP) return 0;
    memset(m->mem + off, 0xFF, len);
    return 1;
}

/* ---------------- tiny static file serving (smoke harness only) --------- */
static const char *s_dist = NULL;

static const char *mime_for(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (!strcmp(dot, ".html")) return "text/html";
    if (!strcmp(dot, ".js")) return "text/javascript";
    if (!strcmp(dot, ".css")) return "text/css";
    if (!strcmp(dot, ".svg")) return "image/svg+xml";
    if (!strcmp(dot, ".ico")) return "image/x-icon";
    return "application/octet-stream";
}

/* Sanitize: no '..', must be under dist, must be a regular file. */
static int dist_open(const char *url)
{
    if (!s_dist || !*s_dist) return -1;
    if (strstr(url, "..")) return -1;
    char path[512];
    snprintf(path, sizeof(path), "%s%s", s_dist, url);
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return -1;
    return open(path, O_RDONLY);
}

/* ---------------- HTTP plumbing ---------------------------------------- */
static int read_all(int fd, char *buf, size_t cap)
{
    size_t n = 0;
    while (n < cap - 1) {
        /* stop early when headers complete and body length satisfied */
        if (n >= 4) {
            char *hdr_end = memmem(buf, n, "\r\n\r\n", 4);
            if (hdr_end) {
                size_t hdr_len = (size_t)(hdr_end - buf) + 4;
                long clen = 0;
                char *cl = strcasestr(buf, "content-length:");
                if (cl && (void *)cl < (void *)hdr_end)
                    clen = strtol(cl + 15, NULL, 10);
                if (n >= hdr_len + (size_t)clen) break;
            }
        }
        ssize_t r = recv(fd, buf + n, cap - 1 - n, 0);
        if (r <= 0) break;
        n += (size_t)r;
    }
    buf[n] = 0;
    return (int)n;
}

static void send_resp(int fd, int status, const char *status_text,
                      const char *ctype, const char *body, size_t len)
{
    char hdr[256];
    int h = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\n"
        "Content-Length: %zu\r\nConnection: close\r\n\r\n",
        status, status_text, ctype, len);
    if (send(fd, hdr, (size_t)h, MSG_NOSIGNAL) < 0) return;
    if (len) {
        size_t off = 0;
        while (off < len) {
            ssize_t w = send(fd, body + off, len - off, MSG_NOSIGNAL);
            if (w <= 0) break;
            off += (size_t)w;
        }
    }
}

static void send_api(int fd, const api_resp_t *r)
{
    const char *st = r->status == 200 ? "OK" : r->status == 400 ? "Bad Request"
                 : r->status == 403 ? "Forbidden" : r->status == 404 ? "Not Found"
                 : "Internal Server Error";
    send_resp(fd, r->status, st, r->content_type ? r->content_type : "application/json",
              r->body, strlen(r->body));
}

static uint32_t g_seed = 42;
static uint32_t g_pre_seed = 3;

/* (Re)initialize the store + ctx to a deterministic state. Called at boot
 * and from GET /__reset (host-only tool endpoint, never part of the
 * protocol) so repeated contract-test runs stay deterministic. */
static void stub_reset(api_ctx_t *ctx, rs_t *ring)
{
    static mem_io_t mem;
    memset(mem.mem, 0xFF, sizeof(mem.mem));
    rs_io_t io = { mem_read, mem_write, mem_erase, &mem, MEM_CAP };
    if (rs_open(ring, &io) != 0) { fprintf(stderr, "ring open failed\n"); exit(1); }
    api_init(ctx, ring, g_seed);
    ctx->wifi_connected = 1;
    api_tick(ctx, 3600u * 1000u); /* 1 h uptime for the health panel */
    for (uint32_t i = 0; i < g_pre_seed; i++) {
        rs_rec_t rec = {0};
        rec.started_epoch = 1758000000u + i * 86400u;
        rec.elapsed_s = 3600u + i * 600u;
        rec.dist_mm = (uint64_t)(25000u + i * 3500u) * 1000u;
        rec.avg_rpm = (uint16_t)(78 + i * 3);
        rec.max_rpm = (uint16_t)(96 + i * 2);
        rec.estimate_basis = 1;
        if (rs_append(ring, &rec) != 0) { fprintf(stderr, "seed append failed\n"); exit(1); }
    }
}

/* Dispatch one request (host tool; not part of the device protocol). */
static void handle_req(int fd, api_ctx_t *ctx, rs_t *ring, char *req)
{
    char method[8], target[256];
    if (sscanf(req, "%7s %255s", method, target) != 2) {
        send_resp(fd, 400, "Bad Request", "application/json", "{}", 2);
        return;
    }
    char *body = strstr(req, "\r\n\r\n");
    body = body ? body + 4 : req + strlen(req);

    api_resp_t r;
    if (!strcmp(method, "GET") && !strcmp(target, "/__reset")) {
        stub_reset(ctx, ring);
        send_resp(fd, 200, "OK", "application/json", "{\"reset\":true}", 15);
        return;
    }
    if (!strcmp(method, "GET") && !strcmp(target, "/api/status")) {
        api_status(ctx, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && !strncmp(target, "/api/sessions?", 14)) {
        uint32_t limit = (uint32_t)strtoul(target + 14 + strlen("limit="), NULL, 10);
        api_sessions(ctx, limit, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && !strcmp(target, "/api/sessions")) {
        api_sessions(ctx, 20, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && !strncmp(target, "/api/sessions/", 14) &&
        isdigit((unsigned char)target[14])) {
        uint32_t id = (uint32_t)strtoul(target + 14, NULL, 10);
        api_session_detail(ctx, id, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && !strcmp(target, "/export.csv")) {
        api_export_csv(ctx, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && !strcmp(target, "/export.json")) {
        api_export_json(ctx, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "POST") && !strcmp(target, "/api/session/toggle")) {
        api_session_toggle(ctx, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "POST") && !strcmp(target, "/api/config")) {
        api_config_post(ctx, body, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && !strcmp(target, "/api/wipe/nonce")) {
        api_wipe_nonce(ctx, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "POST") && !strcmp(target, "/api/data/wipe")) {
        api_wipe_post(ctx, body, &r);
        send_api(fd, &r);
        return;
    }
    if (!strcmp(method, "GET") && (!strcmp(target, "/") ||
        !strncmp(target, "/assets/", 8) || !strcmp(target, "/favicon.ico"))) {
        const char *url = strcmp(target, "/") ? target : "/index.html";
        int f = dist_open(url);
        if (f >= 0) {
            size_t total = 0;
            ssize_t n;
            /* gather file into one buffer (small SPA assets) */
            static char big[512 * 1024];
            while ((n = read(f, big + total, sizeof(big) - total)) > 0)
                total += (size_t)n;
            close(f);
            send_resp(fd, 200, "OK", mime_for(url), big, total);
            return;
        }
        send_resp(fd, 200, "OK", "text/html",
            "<!doctype html><title>gear-miles stub</title>"
            "<p>static dist not configured</p>", 88);
        return;
    }
    send_resp(fd, 404, "Not Found", "application/json",
              "{\"error\":\"not_found\"}", 21);
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    int port = getenv("GM_STUB_PORT") ? atoi(getenv("GM_STUB_PORT")) : 8123;
    if (getenv("GM_STUB_SEED")) g_seed = (uint32_t)strtoul(getenv("GM_STUB_SEED"), NULL, 10);
    if (getenv("GM_STUB_SEEDS")) g_pre_seed = (uint32_t)strtoul(getenv("GM_STUB_SEEDS"), NULL, 10);
    s_dist = getenv("GM_STUB_DIST");

    static api_ctx_t ctx;
    static rs_t ring;
    stub_reset(&ctx, &ring);

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons((uint16_t)port);
    if (bind(srv, (struct sockaddr *)&sa, sizeof(sa)) != 0 || listen(srv, 8) != 0) {
        fprintf(stderr, "bind/listen failed: %s\n", strerror(errno));
        return 1;
    }
    printf("gm-stub ready port=%d sessions=%u proto_v=%d dist=%s\n",
           port, rs_valid_count(&ring), API_PROTO_V, s_dist ? s_dist : "(none)");
    fflush(stdout);

    char req[16384];
    for (;;) {
        int fd = accept(srv, NULL, NULL);
        if (fd < 0) continue;
        if (read_all(fd, req, sizeof(req)) > 0) handle_req(fd, &ctx, &ring, req);
        close(fd);
    }
}
