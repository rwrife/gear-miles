/* Wi-Fi: first-boot captive portal (SSID/pass only path), then station.
 * Zero outbound traffic: SNTP is enabled only if the user later configures
 * time; sessions use monotonic time (architecture §8/README §8). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "mdns.h"
#include "gm_config.h"
#include "storage_nvs.h"

static const char *TAG = "wifi";
static cfg_t *s_cfg;
static int s_failures;

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_failures < 10) { esp_wifi_connect(); s_failures++; }
        /* persistent failure: fall back to the setup AP (reboot to retry) */
        else ESP_LOGW(TAG, "join failed 10x; reboot to re-run setup");
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_failures = 0;
        ip_event_got_ip_t *ev = data;
        ESP_LOGI(TAG, "got ip " IPSTR, IP2STR(&ev->ip_info.ip));
        mdns_init();
        mdns_hostname_set("gearmiles");
        mdns_instance_name_set("gear-miles odometer");
    }
}

static void start_setup_ap_and_portal(cfg_t *cfg);

void wifi_stack_init(cfg_t *cfg)
{
    (void)cfg;
    (void)esp_netif_create_default_wifi_sta();
    (void)esp_netif_create_default_wifi_ap();
    wifi_init_config_t ic = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&ic));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

void wifi_setup_start(cfg_t *cfg)
{
    s_cfg = cfg;
    if (cfg->ssid[0] == 0) {
        ESP_LOGW(TAG, "no stored SSID -> captive setup portal");
        start_setup_ap_and_portal(cfg);
        return;
    }
    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, cfg->ssid, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, cfg->pass, sizeof(wc.sta.password));
    wc.sta.threshold.authmode = cfg->pass[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    esp_wifi_start();
}

/* ---- captive portal: AP + tiny form; writes NVS, reboots ---- */
static httpd_handle_t s_portal;

static esp_err_t portal_form(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_sendstr(req,
        "<!doctype html><meta charset=utf-8>"
        "<form method=post action=/connect>"
        "<label>Wi-Fi <input name=ssid maxlength=32 required></label>"
        "<label>Password <input name=pass type=password maxlength=64></label>"
        "<label>Circumference mm <input name=circ type=number min=300 max=4000 value=2135></label>"
        "<button>Save</button>"
        "<p>Passphrase is sent once to this device on setup-only Wi-Fi "
        "(docs/protocol.md security assumptions). Credentials stay in NVS.</p>"
        "</form>");
}

static void qp(const char *q, const char *k, char *out, size_t cap)
{
    char *p = strstr(q, k);
    out[0] = 0;
    if (!p) return;
    p += strlen(k);
    size_t i = 0;
    while (*p && *p != '&' && i < cap - 1) out[i++] = *p++;
    out[i] = 0;
}

static esp_err_t portal_connect(httpd_req_t *req)
{
    char body[512];
    int total = 0, r;
    while (total < (int)sizeof(body) - 1 &&
           (r = httpd_req_recv(req, body + total, sizeof(body) - 1 - total)) > 0)
        total += r;
    body[total] = 0;
    char ssid[33] = "", pass[65] = "", circ[8] = "";
    qp(body, "ssid=", ssid, sizeof(ssid));
    qp(body, "pass=", pass, sizeof(pass));
    qp(body, "circ=", circ, sizeof(circ));
    if (!ssid[0]) { httpd_resp_sendstr(req, "missing ssid"); return ESP_OK; }
    strlcpy(s_cfg->ssid, ssid, sizeof(s_cfg->ssid));
    strlcpy(s_cfg->pass, pass, sizeof(s_cfg->pass));
    if (circ[0]) {
        unsigned v = strtoul(circ, NULL, 10);
        if (v >= 300 && v <= 4000) s_cfg->circ_mm = (uint16_t)v;
    }
    nvs_cfg_store(s_cfg);
    httpd_resp_sendstr(req, "saved; rebooting onto your Wi-Fi");
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
    return ESP_OK;
}

static void start_setup_ap_and_portal(cfg_t *cfg)
{
    (void)cfg;
    wifi_config_t ap = {0};
    strcpy((char *)ap.ap.ssid, "gearmiles-setup");
    ap.ap.ssid_len = strlen("gearmiles-setup");
    ap.ap.max_connection = 4;
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap);
    esp_wifi_start();

    /* DNS: resolve anything to the portal (real captive-portal redirect) */
    esp_netif_t *apif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (apif) {
        esp_netif_dns_info_t dns = {0};
        dns.ip.type = ESP_IPADDR_TYPE_V4;
        dns.ip.u_addr.ip4.addr = esp_ip4addr_aton("192.168.4.1");
        esp_netif_set_dns_info(apif, ESP_NETIF_DNS_MAIN, &dns);
    }
    httpd_config_t hc = HTTPD_DEFAULT_CONFIG();
    hc.server_port = 80;
    if (httpd_start(&s_portal, &hc) == ESP_OK) {
        httpd_uri_t f = { .uri = "/", .method = HTTP_GET, .handler = portal_form };
        httpd_uri_t c = { .uri = "/connect", .method = HTTP_POST, .handler = portal_connect };
        httpd_register_uri_handler(s_portal, &f);
        httpd_register_uri_handler(s_portal, &c);
    }
    ESP_LOGI(TAG, "setup AP up: connect to 'gearmiles-setup'");
}
