#include "wifi_config_server.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "wifi_config.h"
#include "wifi_credentials.h"
#include "wifi_manager.h"

static const char *TAG = "wifi_config_web";
static httpd_handle_t s_server;

static const char INDEX_HTML[] =
    "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>PulseMon WiFi</title><style>"
    "body{font-family:system-ui,sans-serif;margin:24px;max-width:560px;background:#111;color:#eee}"
    "button,input,select{width:100%;box-sizing:border-box;padding:12px;margin:8px 0;border-radius:6px;border:1px solid #555;background:#222;color:#eee}"
    "button{background:#0b6bcb;border:0;font-weight:700}.row{margin:14px 0}.msg{min-height:24px;color:#9ad}"
    "</style></head><body><h1>PulseMon WiFi</h1><div class=\"msg\" id=\"msg\">Loading...</div>"
    "<div class=\"row\"><button onclick=\"scan()\">Scan SSID</button><select id=\"net\" onchange=\"pick()\"><option value=\"\">Manual SSID</option></select></div>"
    "<input id=\"ssid\" maxlength=\"32\" placeholder=\"SSID\"><input id=\"password\" type=\"password\" maxlength=\"63\" placeholder=\"Password\">"
    "<button onclick=\"save()\">Save and connect</button><button onclick=\"clearWifi()\">Clear saved WiFi</button>"
    "<script>"
    "const msg=document.getElementById('msg'),net=document.getElementById('net'),ssid=document.getElementById('ssid'),password=document.getElementById('password');"
    "function setMsg(t){msg.textContent=t}"
    "function pick(){ssid.value=net.value}"
    "async function status(){try{let r=await fetch('/api/wifi/status');let j=await r.json();setMsg(j.connected?'Connected to '+j.ssid+' '+j.ip:(j.ap_active?'Setup AP active':'Disconnected'))}catch(e){setMsg('Status unavailable')}}"
    "async function scan(){setMsg('Scanning...');let r=await fetch('/api/wifi/scan');if(!r.ok){setMsg('Scan failed');return}let j=await r.json();net.innerHTML='<option value=\"\">Manual SSID</option>';j.networks.forEach(n=>{let o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+' dBm, '+n.auth+')';net.appendChild(o)});setMsg(j.networks.length+' network(s) found')}"
    "async function save(){setMsg('Saving...');let body=new URLSearchParams({ssid:ssid.value,password:password.value});let r=await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});setMsg(r.ok?'Saved, connecting...':await r.text());setTimeout(status,2000)}"
    "async function clearWifi(){let r=await fetch('/api/wifi/clear',{method:'POST'});setMsg(r.ok?'Credentials cleared':'Clear failed');setTimeout(status,1000)}"
    "status();"
    "</script></body></html>";

static void send_text(httpd_req_t *req, int status, const char *text)
{
    const char *status_text = "200 OK";
    if (status == 400) {
        status_text = "400 Bad Request";
    } else if (status == 500) {
        status_text = "500 Internal Server Error";
    }
    httpd_resp_set_status(req, status_text);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, text);
}

static void json_escape(char *dst, size_t dst_len, const char *src)
{
    size_t di = 0;
    if (dst_len == 0) {
        return;
    }
    for (size_t si = 0; src != NULL && src[si] != '\0' && di + 1 < dst_len; si++) {
        char c = src[si];
        if ((c == '"' || c == '\\') && di + 2 < dst_len) {
            dst[di++] = '\\';
            dst[di++] = c;
        } else if ((unsigned char)c >= 0x20) {
            dst[di++] = c;
        }
    }
    dst[di] = '\0';
}

static bool url_decode(char *dst, size_t dst_len, const char *src)
{
    size_t di = 0;
    for (size_t si = 0; src[si] != '\0'; si++) {
        if (di + 1 >= dst_len) {
            return false;
        }
        if (src[si] == '+') {
            dst[di++] = ' ';
        } else if (src[si] == '%' && isxdigit((unsigned char)src[si + 1]) && isxdigit((unsigned char)src[si + 2])) {
            char hex[3] = {src[si + 1], src[si + 2], '\0'};
            dst[di++] = (char)strtol(hex, NULL, 16);
            si += 2;
        } else {
            dst[di++] = src[si];
        }
    }
    dst[di] = '\0';
    return true;
}

static bool form_get(const char *body, const char *key, char *out, size_t out_len)
{
    size_t key_len = strlen(key);
    const char *p = body;
    while (p != NULL && *p != '\0') {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            const char *value = p + key_len + 1;
            const char *end = strchr(value, '&');
            size_t raw_len = end ? (size_t)(end - value) : strlen(value);
            char raw[256];
            if (raw_len >= sizeof(raw)) {
                return false;
            }
            memcpy(raw, value, raw_len);
            raw[raw_len] = '\0';
            return url_decode(out, out_len, raw);
        }
        p = strchr(p, '&');
        if (p != NULL) {
            p++;
        }
    }
    if (out_len > 0) {
        out[0] = '\0';
    }
    return false;
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    pulsemon_wifi_status_t status;
    pulsemon_wifi_manager_get_status(&status);

    char ssid[72];
    json_escape(ssid, sizeof(ssid), status.connected ? status.ssid : "");

    char body[192];
    snprintf(body,
             sizeof(body),
             "{\"mode\":\"%s\",\"connected\":%s,\"ap_active\":%s,\"ssid\":\"%s\",\"ip\":\"%s\"}",
             status.ap_active ? (status.connected ? "apsta" : "ap") : "sta",
             status.connected ? "true" : "false",
             status.ap_active ? "true" : "false",
             ssid,
             status.ip);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t scan_handler(httpd_req_t *req)
{
    pulsemon_wifi_scan_result_t results[PULSEMON_WIFI_SCAN_MAX_RESULTS];
    uint16_t count = 0;
    esp_err_t err = pulsemon_wifi_manager_scan(results, PULSEMON_WIFI_SCAN_MAX_RESULTS, &count);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "scan failed: %s", esp_err_to_name(err));
        send_text(req, 500, "scan failed");
        return ESP_OK;
    }

    char body[2048];
    size_t len = 0;
    len += snprintf(body + len, sizeof(body) - len, "{\"networks\":[");
    for (uint16_t i = 0; i < count && len < sizeof(body); i++) {
        char ssid[72];
        json_escape(ssid, sizeof(ssid), results[i].ssid);
        len += snprintf(body + len,
                        sizeof(body) - len,
                        "%s{\"ssid\":\"%s\",\"rssi\":%d,\"auth\":\"%s\"}",
                        i == 0 ? "" : ",",
                        ssid,
                        (int)results[i].rssi,
                        pulsemon_wifi_authmode_name(results[i].authmode));
    }
    snprintf(body + len, sizeof(body) - len, "]}");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t save_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > 384) {
        send_text(req, 400, "invalid form size");
        return ESP_OK;
    }

    char body[385];
    int total = 0;
    while (total < req->content_len) {
        int received = httpd_req_recv(req, body + total, req->content_len - total);
        if (received <= 0) {
            send_text(req, 400, "form read failed");
            return ESP_OK;
        }
        total += received;
    }
    body[total] = '\0';

    char ssid[PULSEMON_WIFI_SSID_MAX_LEN + 1];
    char password[PULSEMON_WIFI_PASSWORD_MAX_LEN];
    if (!form_get(body, "ssid", ssid, sizeof(ssid))) {
        send_text(req, 400, "ssid required");
        return ESP_OK;
    }
    form_get(body, "password", password, sizeof(password));

    if (!pulsemon_wifi_credentials_validate(ssid, password)) {
        send_text(req, 400, "invalid ssid or password");
        return ESP_OK;
    }

    esp_err_t err = pulsemon_wifi_manager_apply_credentials(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "wifi credential apply failed: %s", esp_err_to_name(err));
        send_text(req, 500, "save failed");
        return ESP_OK;
    }

    send_text(req, 200, "saved");
    return ESP_OK;
}

static esp_err_t clear_handler(httpd_req_t *req)
{
    esp_err_t err = pulsemon_wifi_manager_clear_credentials();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "wifi credential clear failed: %s", esp_err_to_name(err));
        send_text(req, 500, "clear failed");
        return ESP_OK;
    }
    send_text(req, 200, "cleared");
    return ESP_OK;
}

esp_err_t pulsemon_wifi_config_server_start(void)
{
    if (s_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.stack_size = 6144;
    config.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "http server start failed: %s", esp_err_to_name(err));
        return err;
    }

    const httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = index_handler};
    const httpd_uri_t status_uri = {.uri = "/api/wifi/status", .method = HTTP_GET, .handler = status_handler};
    const httpd_uri_t scan_uri = {.uri = "/api/wifi/scan", .method = HTTP_GET, .handler = scan_handler};
    const httpd_uri_t save_uri = {.uri = "/api/wifi", .method = HTTP_POST, .handler = save_handler};
    const httpd_uri_t clear_uri = {.uri = "/api/wifi/clear", .method = HTTP_POST, .handler = clear_handler};
    const httpd_uri_t portal_uri = {.uri = "/*", .method = HTTP_GET, .handler = index_handler};

    httpd_register_uri_handler(s_server, &index_uri);
    httpd_register_uri_handler(s_server, &status_uri);
    httpd_register_uri_handler(s_server, &scan_uri);
    httpd_register_uri_handler(s_server, &save_uri);
    httpd_register_uri_handler(s_server, &clear_uri);
    httpd_register_uri_handler(s_server, &portal_uri);

    ESP_LOGI(TAG, "wifi config server started");
    return ESP_OK;
}
