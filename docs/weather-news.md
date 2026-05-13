# Weather and news modules

The ESP32-S3 firmware includes autonomous information features that do not depend on the Linux backend once Wi-Fi and keys are configured.

## Weather

Weather data is fetched from OpenWeather by the firmware.

The firmware stores OpenWeather configuration in NVS and updates generated UI variables for:

- current local time;
- local date;
- current temperature;
- weather condition;
- rolling forecast labels;
- weather icons loaded from SD card assets.

The OpenWeather key must not be logged or returned by configuration endpoints.

## Weather icons

Weather icons are loaded from the SD card mount point:

- `/sdcard/icon_150.bin` for the main current-weather icon;
- `/sdcard/icon_50.bin` for rolling forecast icons.

Startup diagnostics should validate SD mounting and icon-bin decoding.

## News provider

News headlines are fetched directly by the ESP32-S3 from GNews.

The provider is GNews only. NewsAPI is not used as fallback.

Endpoint:

```text
https://gnews.io/api/v4/top-headlines
```

Query parameters:

| Parameter | Value |
|---|---|
| `category` | `general` |
| `lang` | `fr` |
| `country` | `fr` |
| `max` | `5` |
| `from` | current UTC time minus 15 days, ISO formatted |

Authentication header:

```text
X-Api-Key: <GNews key from NVS>
```

The key must not be placed in the URL during normal operation.

## News NVS storage

Recommended namespace: `news`.

Recommended keys:

| Key | Purpose |
|---|---|
| `provider` | fixed provider name, `gnews` |
| `gnews_key` | GNews API key |
| `enabled` | module enable flag |
| `refresh_min` | refresh interval |
| `category` | GNews category |
| `lang` | headline language |
| `country` | country filter |
| `max_items` | maximum requested articles |
| `slide_speed` | ticker speed |
| `max_age_days` | maximum article age |
| `last_ok_ts` | last successful fetch timestamp |
| `last_error` | compact last error code |

Default behavior:

- refresh every 30 minutes;
- request up to 5 articles;
- reject articles older than 15 days;
- keep the last valid headline in cache;
- leave the line empty if no valid content exists.

## News request preconditions

Before calling GNews, the firmware must have:

- Wi-Fi connected;
- DNS working;
- valid SNTP time;
- GNews key stored in NVS;
- news module enabled;
- refresh interval elapsed;
- no active backoff.

SNTP time is required for TLS validation and for computing the `from` parameter.

## Article validation

A valid article must have:

- a non-empty title longer than 10 characters;
- a parsable `publishedAt` value;
- publication time within the maximum age window;
- `lang=fr` when the field is present;
- valid UTF-8 text.

The UI uses cleaned titles only. It does not display article descriptions, content, URLs or images.

## Information line priority

The information line uses this priority:

1. weather alert;
2. valid news headline;
3. cached news headline;
4. empty line.

Weather alerts always override news. Technical errors should remain in diagnostics and logs, not in the normal information line.

## Backoff

Recommended retry behavior:

- temporary network, TLS or server error: retry after the normal long interval;
- rate limit: long backoff;
- quota or authorization error: no aggressive retry;
- parse or empty result: preserve cache and retry later.
