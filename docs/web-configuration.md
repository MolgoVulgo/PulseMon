# ESP32 web configuration

The ESP32-S3 firmware exposes a small local web interface on its HTTP configuration server. This portal is for device setup, not for a rich monitoring dashboard.

## Pages

- `GET /` serves the PulseMon configuration page.
- `GET /wifi` serves the Wi-Fi configuration page.

Unknown `GET` paths can fall back to `/` to support captive portal behavior.

## Configuration API

The firmware configuration API includes:

- `GET /api/config`;
- `POST /api/config`;
- `POST /api/config/clear`.

`POST /api/config` accepts `application/x-www-form-urlencoded` fields such as:

- `openweather_key`;
- `clear_openweather_key`;
- `gnews_key`;
- `clear_gnews_key`;
- `news_max_items`;
- `news_slide_speed`;
- `gmt_offset_min`;
- `openweather_city_id`;
- `language`.

## NVS namespaces

Main PulseMon configuration is stored in `pulsemon_cfg`:

| Key | Purpose |
|---|---|
| `ow_key` | OpenWeather API key |
| `gmt_min` | GMT offset in minutes |
| `ow_city` | OpenWeather city id |
| `lang` | UI/weather language |

News configuration is stored in `news`:

| Key | Purpose |
|---|---|
| `provider` | active provider, fixed to GNews |
| `gnews_key` | GNews key |
| `enabled` | news module enable flag |
| `refresh_min` | refresh interval |
| `category` | GNews category |
| `lang` | GNews language |
| `country` | GNews country |
| `max_items` | maximum article count |
| `slide_speed` | LVGL circular scroll speed |
| `max_age_days` | maximum article age |
| `last_ok_ts` | last successful fetch timestamp |
| `last_error` | compact last error code |

## Secret handling

`GET /api/config` must not return API keys. It should only return presence flags such as:

- `openweather_key_set`;
- `gnews_key_set`.

Reset operations must clear the related keys from NVS.
