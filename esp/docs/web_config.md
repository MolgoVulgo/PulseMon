# PulseMon ESP web configuration

## Scope

The ESP32-S3 firmware exposes a small local web interface on the existing HTTP
configuration server. It is intended for device setup only, not for a rich
dashboard.

## Pages

- `GET /` serves the PulseMon configuration page.
- `GET /wifi` serves the Wi-Fi configuration page.

Unknown `GET` paths still fall back to `/` for captive portal behavior.

## Configuration API

- `GET /api/config`
- `POST /api/config`
- `POST /api/config/clear`

`POST /api/config` accepts `application/x-www-form-urlencoded` fields:

- `openweather_key`
- `clear_openweather_key`
- `gmt_offset_min`
- `openweather_city_id`
- `language`

## NVS

PulseMon configuration is stored in the `pulsemon_cfg` namespace:

- `ow_key`: OpenWeather API key.
- `gmt_min`: GMT offset in minutes.
- `ow_city`: OpenWeather city id.
- `lang`: UI/weather language.

The OpenWeather key must not be logged or returned by the API. `GET /api/config`
only reports `openweather_key_set`.

## Meteo UI integration

The firmware consumes the stored configuration through `pulsemon_meteo_service`:

- time/date are updated from SNTP plus `gmt_min`;
- current weather is fetched from OpenWeather `/data/2.5/weather`;
- rolling forecast labels are fetched from OpenWeather `/data/2.5/forecast`;
- `lang` is used for OpenWeather descriptions and local weekday/month labels.

The current implementation updates the existing generated EEZ variables only:

- `ui_meteo_houre`
- `ui_meteo_date`
- `ui_meteo_temp`
- `ui_meteo_condition`
- `ui_meteo_fd1` to `ui_meteo_fd6`
- `ui_meteo_ft1` to `ui_meteo_ft6`
- `ui_meteo_img`
- `ui_meteo_fi1` to `ui_meteo_fi6`

## Weather icons

Weather icons are loaded from the SD card mount point:

- `/sdcard/icon_150.bin` for the main current-weather icon.
- `/sdcard/icon_50.bin` for the six rolling forecast icons.

At startup, `pulsemon_weather_icons_init()` mounts the SD card, logs the file
list from `/sdcard`, then logs the SVG2BIN index entries found in both icon bin
files. This is the runtime validation path for SD access and icon-bin decoding.
