# Weather and news

The ESP32-S3 fetches weather and news directly. These services do not use the Linux backend.

## Weather configuration

Stored in NVS namespace `pulsemon_cfg`:

- `ow_key`;
- `gmt_min`;
- `ow_city`;
- `lang`.

Supported weather languages are `fr`, `en`, `de`, `es` and `it`. The default GMT offset is `+60 minutes`; the default language is `fr`.

## OpenWeather transport

The firmware uses these HTTPS endpoints:

```text
https://api.openweathermap.org/data/2.5/weather
https://api.openweathermap.org/data/3.0/onecall
https://api.openweathermap.org/data/2.5/forecast
```

The API key is placed in the `appid` query parameter required by OpenWeather. TLS certificates are validated through the ESP-IDF certificate bundle. The firmware does not log the request URL or the key.

Weather refresh is scheduled every 30 minutes. The service keeps the last valid weather snapshot when a request, TLS validation or parsing step fails.

## HTTPS coordination and memory

Weather and GNews share `pulsemon_https_gate`, a firmware mutex that prevents simultaneous external HTTPS/TLS sessions. Weather holds the gate across its complete current-plus-forecast update, including the forecast fallback path. GNews waits for the same gate before its request.

After Wi-Fi obtains an IP address, startup requests are staggered by one-shot timers: backend polling starts after 0.5 s, Weather after 1.5 s cumulative and GNews after 4.5 s cumulative. The timer callback only schedules service work; network requests run in their service tasks.

Weather allocates its 32 KiB body explicitly from PSRAM and GNews does the same for its 16 KiB body. MbedTLS uses dynamic buffers with the normal allocator policy, while 32 KiB of internal memory is reserved for INTERNAL/DMA-only needs. This arrangement is intended to preserve internal/DMA headroom during TLS handshakes without allowing Weather and GNews to compete for it concurrently.

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

TLS certificates are validated through the ESP-IDF certificate bundle.

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
