/* LAN HTTP API server. Routes map onto gm_api handlers. No outbound
 * internet traffic; LAN-only per docs/protocol.md threat model. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "gm_api.h"
#include "http_server.h"

static const char *TAG = "http";
static api_ctx_t *s_ctx;

static esp_err_t send_resp(httpd_req_t *req, const api_resp_t *r)
{
    httpd_resp_set_status(req, r->status == 200 ? "200 OK"
                          : r->status == 400 ? "400 Bad Request"
                          : r->status == 403 ? "403 Forbidden"
                          : r->status == 404 ? "404 Not Found" : "500 Err");
    httpd_resp_set_type(req, r->content_type ? r->content_type : "application/json");
    return httpd_resp_send(req, r->body, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t h_status(httpd_req_t *req)
{
    api_resp_t r; api_status(s_ctx, &r);
    return send_resp(req, &r);
}

static esp_err_t h_sessions(httpd_req_t *req)
{
    api_resp_t r;
    uint32_t limit = 20;
    char q[64];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK) {
        char v[16];
        if (httpd_query_key_value(q, "limit", v, sizeof(v)) == ESP_OK)
            limit = (uint32_t)strtoul(v, NULL, 10);
    }
    api_sessions(s_ctx, limit, &r);
    return send_resp(req, &r);
}

static esp_err_t h_session_detail(httpd_req_t *req)
{
    uint32_t id = (uint32_t)strtoul(req->uri + strlen("/api/sessions/"), NULL, 10);
    api_resp_t r; api_session_detail(s_ctx, id, &r);
    return send_resp(req, &r);
}

static esp_err_t read_body(httpd_req_t *req, char *buf, size_t cap)
{
    int total = 0, r;
    while (total < (int)cap - 1 &&
           (r = httpd_req_recv(req, buf + total, cap - 1 - total)) > 0)
        total += r;
    buf[total] = 0;
    return total;
}

static esp_err_t h_config(httpd_req_t *req)
{
    char body[512];
    read_body(req, body, sizeof(body));
    api_resp_t r; api_config_post(s_ctx, body, &r);
    if (r.status == 200) nvs_cfg_store(&s_ctx->cfg); /* persist on accept */
    return send_resp(req, &r);
}

static esp_err_t h_toggle(httpd_req_t *req)
{
    api_resp_t r; api_session_toggle(s_ctx, &r);
    return send_resp(req, &r);
}

static esp_err_t h_nonce(httpd_req_t *req)
{
    api_resp_t r; api_wipe_nonce(s_ctx, &r);
    return send_resp(req, &r);
}

static esp_err_t h_wipe(httpd_req_t *req)
{
    char body[128];
    read_body(req, body, sizeof(body));
    api_resp_t r; api_wipe_post(s_ctx, body, &r);
    if (r.status == 200) nvs_cfg_wipe();
    return send_resp(req, &r);
}

static esp_err_t h_export_csv(httpd_req_t *req)
{
    api_resp_t r; api_export_csv(s_ctx, &r);
    return send_resp(req, &r);
}

/* Placeholder root: the real dashboard SPA is issue #7 and will be
 * embedded as gzipped assets; the API contract is complete without it. */
static esp_err_t h_root(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_sendstr(req,
        "<!doctype html><meta charset=utf-8>"
        "<title>gear-miles</title><h1>gear-miles</h1>"
        "<p>Dashboard pending issue #7. API: /api/status</p>");
}

void http_server_start(api_ctx_t *ctx)
{
    s_ctx = ctx;
    httpd_handle_t srv = NULL;
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    if (httpd_start(&srv, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "httpd start failed");
        return;
    }
    struct { const char *uri; httpd_method_t m; esp_err_t (*h)(httpd_req_t*); } routes[] = {
        {"/api/status", HTTP_GET, h_status},
        {"/api/sessions", HTTP_GET, h_sessions},
        {"/api/sessions/", HTTP_GET, h_session_detail},
        {"/export.csv", HTTP_GET, h_export_csv},
        {"/api/session/toggle", HTTP_POST, h_toggle},
        {"/api/config", HTTP_POST, h_config},
        {"/api/wipe/nonce", HTTP_GET, h_nonce},
        {"/api/data/wipe", HTTP_POST, h_wipe},
        {"/", HTTP_GET, h_root},
    };
    for (size_t i = 0; i < sizeof(routes)/sizeof(routes[0]); i++) {
        httpd_uri_t u = { .uri = routes[i].uri, .method = routes[i].m,
                          .handler = routes[i].h };
        httpd_register_uri_handler(srv, &u);
    }
    ESP_LOGI(TAG, "LAN API listening on :80 (no outbound internet)");
}
