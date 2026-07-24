# ESP32 local web configuration

The firmware starts an ESP-IDF HTTP configuration server during normal startup. The server is used for Wi-Fi, weather and news settings; it is not a monitoring dashboard.

## Pages and routes

- `GET /` — weather/news configuration page;
- `GET /wifi` — Wi-Fi configuration page;
- `GET /api/config` — non-secret weather/news state;
- `POST /api/config` — update weather/news settings;
- `POST /api/config/clear` — clear weather and news namespaces;
- `GET /api/wifi/status` — current station/AP status;
- `GET /api/wifi/scan` — scan visible networks;
- `POST /api/wifi` — save and apply station credentials;
- `POST /api/wifi/clear` — clear credentials and enable the setup AP;
- unknown GET paths — serve the main page for captive-portal behavior.

## Configuration fields

`POST /api/config` accepts form fields:

- `openweather_key` and `clear_openweather_key`;
- `gnews_key` and `clear_gnews_key`;
- required `gmt_offset_min`;
- optional `openweather_city_id`;
- required `language`;
- optional `news_max_items`;
- optional `news_slide_speed`.

`GET /api/config` returns key-presence flags, not key values.

## Wi-Fi AP behavior

The setup AP is `PulseMon-Setup` with an empty password in the current configuration. It is enabled when credentials are missing or the station connection repeatedly fails, and disabled after a successful station connection.

The HTTP server and captive DNS are started independently from AP activation. The server has no application-level authentication in the current implementation. This is acceptable only within the intended trusted local network and should remain a conscious limitation.
