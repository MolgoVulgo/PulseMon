#include "wifi_config_server.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "pulsemon_meteo_service.h"
#include "pulsemon_settings.h"
#include "wifi_config.h"
#include "wifi_credentials.h"
#include "wifi_manager.h"

static const char *TAG = "wifi_config_web";
static httpd_handle_t s_server;

static const char CONFIG_HTML[] =
    "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>PulseMon Config</title><style>"
    "body{font-family:system-ui,sans-serif;margin:24px;max-width:640px;background:#111;color:#eee}"
    "nav a{color:#9ad;margin-right:16px}label{display:block;margin-top:14px;color:#ccc}"
    "button,input,select{width:100%;box-sizing:border-box;padding:12px;margin:8px 0;border-radius:6px;border:1px solid #555;background:#222;color:#eee}"
    "button{background:#0b6bcb;border:0;font-weight:700}.row{margin:14px 0}.msg{min-height:24px;color:#9ad}.hint{color:#999;font-size:14px}"
    "</style></head><body><nav><a href=\"/\">Config</a><a href=\"/wifi\">WiFi</a></nav><h1>PulseMon</h1><div class=\"msg\" id=\"msg\">Loading...</div>"
    "<label>OpenWeather key</label><input id=\"openweather_key\" maxlength=\"96\" type=\"password\" placeholder=\"Leave empty to keep saved key\">"
    "<label><input id=\"clear_openweather_key\" type=\"checkbox\" style=\"width:auto\"> Clear saved OpenWeather key</label>"
    "<label>GMT offset</label><select id=\"gmt_offset_min\">"
    "<option value=\"-720\">GMT-12:00</option><option value=\"-660\">GMT-11:00</option><option value=\"-600\">GMT-10:00</option>"
    "<option value=\"-540\">GMT-09:00</option><option value=\"-480\">GMT-08:00</option><option value=\"-420\">GMT-07:00</option>"
    "<option value=\"-360\">GMT-06:00</option><option value=\"-300\">GMT-05:00</option><option value=\"-240\">GMT-04:00</option>"
    "<option value=\"-180\">GMT-03:00</option><option value=\"-120\">GMT-02:00</option><option value=\"-60\">GMT-01:00</option>"
    "<option value=\"0\">GMT+00:00</option><option value=\"60\">GMT+01:00</option><option value=\"120\">GMT+02:00</option>"
    "<option value=\"180\">GMT+03:00</option><option value=\"240\">GMT+04:00</option><option value=\"300\">GMT+05:00</option>"
    "<option value=\"330\">GMT+05:30</option><option value=\"360\">GMT+06:00</option><option value=\"420\">GMT+07:00</option>"
    "<option value=\"480\">GMT+08:00</option><option value=\"540\">GMT+09:00</option><option value=\"600\">GMT+10:00</option>"
    "<option value=\"660\">GMT+11:00</option><option value=\"720\">GMT+12:00</option><option value=\"780\">GMT+13:00</option>"
    "<option value=\"840\">GMT+14:00</option></select>"
    "<label>OpenWeather city id</label><input id=\"openweather_city_id\" inputmode=\"numeric\" pattern=\"[0-9]*\" placeholder=\"Example: 2988507\">"
    "<label>Language</label><select id=\"language\"><option value=\"fr\">fr</option><option value=\"en\">en</option><option value=\"de\">de</option><option value=\"es\">es</option><option value=\"it\">it</option></select>"
    "<button onclick=\"saveConfig()\">Save configuration</button><button onclick=\"clearConfig()\">Reset PulseMon config</button><p class=\"hint\" id=\"keyState\"></p>"
    "<script>"
    "const msg=document.getElementById('msg'),key=document.getElementById('openweather_key'),clearKey=document.getElementById('clear_openweather_key'),gmt=document.getElementById('gmt_offset_min'),city=document.getElementById('openweather_city_id'),lang=document.getElementById('language'),keyState=document.getElementById('keyState');"
    "function setMsg(t){msg.textContent=t}"
    "async function loadConfig(){try{let r=await fetch('/api/config');let j=await r.json();gmt.value=String(j.gmt_offset_min);city.value=j.openweather_city_id?String(j.openweather_city_id):'';lang.value=j.language||'fr';keyState.textContent=j.openweather_key_set?'OpenWeather key saved':'No OpenWeather key saved';setMsg('Ready')}catch(e){setMsg('Config unavailable')}}"
    "async function saveConfig(){setMsg('Saving...');let body=new URLSearchParams({gmt_offset_min:gmt.value,openweather_city_id:city.value,language:lang.value});if(key.value)body.set('openweather_key',key.value);if(clearKey.checked)body.set('clear_openweather_key','1');let r=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});setMsg(r.ok?'Saved':await r.text());key.value='';clearKey.checked=false;loadConfig()}"
    "async function clearConfig(){let r=await fetch('/api/config/clear',{method:'POST'});setMsg(r.ok?'PulseMon config reset':'Reset failed');loadConfig()}"
    "loadConfig();"
    "</script></body></html>";

static const char WIFI_HTML[] =
    "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>PulseMon WiFi</title><style>"
    "body{font-family:system-ui,sans-serif;margin:24px;max-width:560px;background:#111;color:#eee}"
    "nav a{color:#9ad;margin-right:16px}"
    "button,input,select{width:100%;box-sizing:border-box;padding:12px;margin:8px 0;border-radius:6px;border:1px solid #555;background:#222;color:#eee}"
    "button{background:#0b6bcb;border:0;font-weight:700}.row{margin:14px 0}.msg{min-height:24px;color:#9ad}"
    "</style></head><body><nav><a href=\"/\">Config</a><a href=\"/wifi\">WiFi</a></nav><h1>PulseMon WiFi</h1><div class=\"msg\" id=\"msg\">Loading...</div>"
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

static bool form_get_i32(const char *body, const char *key, int32_t *out)
{
    char value[24];
    char *end = NULL;

    if (out == NULL || !form_get(body, key, value, sizeof(value))) {
        return false;
    }
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        return false;
    }
    if (parsed < INT32_MIN || parsed > INT32_MAX) {
        return false;
    }
    *out = (int32_t)parsed;
    return true;
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, CONFIG_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t wifi_page_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, WIFI_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    pulsemon_settings_t settings;
    esp_err_t err = pulsemon_settings_load(&settings);
    if (err != ESP_OK && err != ESP_ERR_INVALID_SIZE) {
        ESP_LOGW(TAG, "settings load failed: %s", esp_err_to_name(err));
        send_text(req, 500, "settings load failed");
        return ESP_OK;
    }

    char language[16];
    json_escape(language, sizeof(language), settings.language);

    char body[192];
    snprintf(body,
             sizeof(body),
             "{\"openweather_key_set\":%s,\"gmt_offset_min\":%d,\"openweather_city_id\":%lu,\"language\":\"%s\"}",
             settings.openweather_key[0] != '\0' ? "true" : "false",
             (int)settings.gmt_offset_min,
             (unsigned long)settings.openweather_city_id,
             language);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t config_save_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > 640) {
        send_text(req, 400, "invalid form size");
        return ESP_OK;
    }

    char body[641];
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

    pulsemon_settings_t settings;
    esp_err_t err = pulsemon_settings_load(&settings);
    if (err != ESP_OK && err != ESP_ERR_INVALID_SIZE) {
        ESP_LOGW(TAG, "settings load before save failed: %s", esp_err_to_name(err));
        send_text(req, 500, "settings load failed");
        return ESP_OK;
    }

    char openweather_key[PULSEMON_OPENWEATHER_KEY_MAX_LEN + 1];
    char clear_openweather_key[4];
    if (form_get(body, "clear_openweather_key", clear_openweather_key, sizeof(clear_openweather_key)) &&
        strcmp(clear_openweather_key, "1") == 0) {
        settings.openweather_key[0] = '\0';
    } else if (form_get(body, "openweather_key", openweather_key, sizeof(openweather_key)) && openweather_key[0] != '\0') {
        snprintf(settings.openweather_key, sizeof(settings.openweather_key), "%s", openweather_key);
    }

    int32_t gmt_offset = 0;
    if (!form_get_i32(body, "gmt_offset_min", &gmt_offset)) {
        send_text(req, 400, "gmt_offset_min required");
        return ESP_OK;
    }
    if (gmt_offset < -720 || gmt_offset > 840) {
        send_text(req, 400, "invalid gmt_offset_min");
        return ESP_OK;
    }
    settings.gmt_offset_min = (int16_t)gmt_offset;

    char city_value[24];
    if (form_get(body, "openweather_city_id", city_value, sizeof(city_value)) && city_value[0] != '\0') {
        char *end = NULL;
        long city_id = strtol(city_value, &end, 10);
        if (end == city_value || *end != '\0' || city_id < 0 || city_id > UINT32_MAX) {
            send_text(req, 400, "invalid openweather_city_id");
            return ESP_OK;
        }
        settings.openweather_city_id = (uint32_t)city_id;
    } else {
        settings.openweather_city_id = 0;
    }

    if (!form_get(body, "language", settings.language, sizeof(settings.language))) {
        send_text(req, 400, "language required");
        return ESP_OK;
    }

    if (!pulsemon_settings_validate(&settings)) {
        send_text(req, 400, "invalid configuration");
        return ESP_OK;
    }

    err = pulsemon_settings_save(&settings);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "settings save failed: %s", esp_err_to_name(err));
        send_text(req, 500, "settings save failed");
        return ESP_OK;
    }

    send_text(req, 200, "saved");
    pulsemon_meteo_service_request_update();
    return ESP_OK;
}

static esp_err_t config_clear_handler(httpd_req_t *req)
{
    esp_err_t err = pulsemon_settings_clear();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "settings clear failed: %s", esp_err_to_name(err));
        send_text(req, 500, "settings clear failed");
        return ESP_OK;
    }
    send_text(req, 200, "cleared");
    pulsemon_meteo_service_request_update();
    return ESP_OK;
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
    const httpd_uri_t wifi_page_uri = {.uri = "/wifi", .method = HTTP_GET, .handler = wifi_page_handler};
    const httpd_uri_t config_get_uri = {.uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler};
    const httpd_uri_t config_save_uri = {.uri = "/api/config", .method = HTTP_POST, .handler = config_save_handler};
    const httpd_uri_t config_clear_uri = {.uri = "/api/config/clear", .method = HTTP_POST, .handler = config_clear_handler};
    const httpd_uri_t status_uri = {.uri = "/api/wifi/status", .method = HTTP_GET, .handler = status_handler};
    const httpd_uri_t scan_uri = {.uri = "/api/wifi/scan", .method = HTTP_GET, .handler = scan_handler};
    const httpd_uri_t save_uri = {.uri = "/api/wifi", .method = HTTP_POST, .handler = save_handler};
    const httpd_uri_t clear_uri = {.uri = "/api/wifi/clear", .method = HTTP_POST, .handler = clear_handler};
    const httpd_uri_t portal_uri = {.uri = "/*", .method = HTTP_GET, .handler = index_handler};

    httpd_register_uri_handler(s_server, &index_uri);
    httpd_register_uri_handler(s_server, &wifi_page_uri);
    httpd_register_uri_handler(s_server, &config_get_uri);
    httpd_register_uri_handler(s_server, &config_save_uri);
    httpd_register_uri_handler(s_server, &config_clear_uri);
    httpd_register_uri_handler(s_server, &status_uri);
    httpd_register_uri_handler(s_server, &scan_uri);
    httpd_register_uri_handler(s_server, &save_uri);
    httpd_register_uri_handler(s_server, &clear_uri);
    httpd_register_uri_handler(s_server, &portal_uri);

    ESP_LOGI(TAG, "wifi config server started");
    return ESP_OK;
}
