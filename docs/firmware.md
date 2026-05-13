# ESP32-S3 firmware

The firmware is built with ESP-IDF and LVGL. It displays backend metrics on a local screen and provides autonomous device-side functions.

## Main responsibilities

The firmware must:

- connect to Wi-Fi;
- open a setup access point and captive portal when needed;
- poll the backend API;
- parse JSON safely;
- cache the latest valid values;
- update LVGL variables and screens;
- preserve display state during transient network failures;
- show backend connectivity and freshness state;
- keep weather and news services independent from backend availability.

## Screens

The active screen set includes:

- Main: CPU, RAM and GPU summary from `/api/v1/dashboard`;
- GPU: AMD GPU detail from `/api/v1/gpu/dashboard`;
- Weather: autonomous weather information and information line.

Fan UI sources can remain present, but navigation and polling must reflect the active firmware behavior.

## Polling

The main poller is controlled by `PULSEMON_DASHBOARD_POLL_MS`, defaulting to `1000` ms.

Current runtime behavior:

- Main screen fetches `/api/v1/dashboard`;
- GPU screen fetches `/api/v1/gpu/dashboard`;
- Weather screen keeps backend polling minimal and does not use backend data for weather;
- graph data can be fed from local display samples when backend history is not consumed.

## JSON parsing

The parser should use this priority for metric values:

1. `value_display`;
2. `value_raw`;
3. direct scalar value where compatibility is needed.

Invalid or incomplete JSON must be rejected without clearing the last valid UI values.

## Navigation

The documented swipe model is:

- Main swipe left to GPU;
- GPU swipe right to Main;
- GPU swipe left to Weather;
- Weather swipe right to GPU.

## Generated UI files

Files generated under `src/ui/` must not be edited directly. UI changes that require new variables, glyphs or layout updates must be made in the source UI project and regenerated.

## Failure behavior

On backend fetch failure, the firmware should:

- retain the last valid display values;
- mark backend state as offline or stale;
- avoid blocking the UI thread;
- retry according to the nominal polling cycle;
- keep autonomous weather/news features operational when Wi-Fi and keys are valid.
