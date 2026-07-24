# Weather and news

The ESP32-S3 fetches weather and news directly. These services do not use the Linux backend.

## Weather configuration

Stored in NVS namespace `pulsemon_cfg`:

- `ow_key`;
- `gmt_min`;
- `ow_city`;
- `lang`.

Supported weather languages are `fr`, `en`, `de`, `es` and `it`. The default GMT offset is `+60 minutes`; the default language is `fr`.

## Current OpenWeather transport

The current firmware builds these URLs:

```text
http://api.openweathermap.org/data/2.5/weather
http://api.openweathermap.org/data/3.0/onecall
http://api.openweathermap.org/data/2.5/forecast
```

The API key is placed in the `appid` query parameter. This is a known defect because the requests use plain HTTP. It remains documented as current behavior until the client is migrated to HTTPS with certificate validation.

Weather refresh is scheduled every 30 minutes. The service keeps the last valid weather snapshot when a refresh fails.

## Weather icons

Binary icons are loaded from the SD card using `/sdcard/icon_150.bin` and `/sdcard/icon_50.bin`.

## GNews

GNews uses:

```text
https://gnews.io/api/v4/top-headlines
```

Authentication:

```text
X-Api-Key: <key stored in NVS>
```

Default settings:

- enabled;
- refresh every 30 minutes;
- category `general`;
- language `fr`;
- country `fr`;
- maximum 5 items;
- maximum article age 15 days;
- ticker speed 35.

The service requires Wi-Fi, DNS, valid SNTP time and a stored GNews key. It validates article title, publication date, age, optional language and UTF-8 content, then keeps valid headlines in a local cache.

## Information line priority

1. weather alert;
2. current valid news item;
3. cached news item;
4. empty line.
