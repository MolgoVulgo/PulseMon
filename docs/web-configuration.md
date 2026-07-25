# ESP32 local web configuration

The firmware exposes an ESP-IDF HTTP configuration server only while the setup AP is active. The server is used for the backend endpoint, Wi-Fi, weather and news settings; it is not a monitoring dashboard or a permanent LAN service.

## Pages and routes

- `GET /` — weather/news configuration page;
- `GET /wifi` — Wi-Fi configuration page;
- `GET /api/config` — backend endpoint and non-secret weather/news state;
- `POST /api/config` — update backend, weather and news settings;
- `POST /api/config/clear` — clear backend, weather and news namespaces and restore endpoint fallback;
- `GET /api/wifi/status` — current station/AP status;
- `GET /api/wifi/scan` — scan visible networks;
- `POST /api/wifi` — save and apply station credentials;
- `POST /api/wifi/clear` — clear credentials and enable the setup AP;
- unknown GET paths — serve the main page for captive-portal behavior.

## Configuration fields

`POST /api/config` accepts form fields:

- required `backend_host`, validated as an IPv4 address or DNS/mDNS hostname;
- required `backend_port`, integer `1..65535`;
- `openweather_key` and `clear_openweather_key`;
- `gnews_key` and `clear_gnews_key`;
- required `gmt_offset_min`;
- optional `openweather_city_id`;
- required `language`;
- optional `news_max_items`;
- optional `news_slide_speed`.

`GET /api/config` returns `backend_host`, `backend_port` and key-presence flags, never key values. The endpoint is stored in NVS namespace `pulsemon_api`. A successful save reloads the active backend target immediately; no reboot is required.

## Wi-Fi AP behavior

The setup AP is `PulseMon-Setup` with an empty password in the current configuration. It is enabled automatically when credentials are missing or the station connection repeatedly fails. That automatic AP is disabled after a successful station connection; a manually opened window remains active until its timeout.

`WIFI_EVENT_AP_START` starts the HTTP server and captive DNS. `WIFI_EVENT_AP_STOP` stops captive DNS first and then the HTTP server. Captive DNS binds only to `192.168.4.1`; it no longer listens on all interfaces. Every HTTP handler also checks that the setup AP is active and returns `503 Service Unavailable` otherwise.

The setup AP has no application-level authentication and currently uses an empty Wi-Fi password. Exposure is therefore intentionally limited to periods when configuration mode is required. When the firmware is connected in normal station mode with the AP disabled, the portal is not available through the station LAN address.

An explicit local trigger is available on every active screen: hold the top-left corner for five seconds. The firmware keeps the current station connection and opens `PulseMon-Setup` for ten minutes. The HTTP server and captive DNS follow the resulting AP lifecycle. When the ten-minute window expires, the AP closes automatically if the station is connected; if station connectivity is unavailable, the normal fallback rules keep configuration access available. The trigger does not erase credentials or change NVS settings.
