# PulseMon ESP32-S3 agent context

## Source of truth

Use the supplied snapshot and the firmware files actually present under `esp/`. Read `../README.md`, `../docs/firmware.md`, `../docs/configuration.md`, `platformio.ini`, `src/main.c` and the exact module being changed.

## Build

```text
pulsmon-esp32s3-display      -> default release environment
pulsmon-esp32s3-display-dev  -> debug environment
```

Do not build, flash or monitor without explicit user instruction.

## Active runtime

- active screens: Main, GPU and Weather;
- backend polling: main dashboard outside GPU screen, GPU dashboard on GPU screen;
- last valid values are preserved on failures;
- backend endpoint is compiled in `src/pulsemon_api_config.h`;
- no backend endpoint or backend API key is stored in NVS;
- Wi-Fi, OpenWeather and GNews settings are stored in NVS;
- OpenWeather currently uses HTTP; GNews uses HTTPS.

## FAN status

FAN code is retained but inactive. Do not reactivate, delete or repair it unless the user explicitly scopes that work. `SCREEN_ID_FAN` is not a navigation target and the poller does not fetch FAN data.

## EEZ ownership

```text
src/ui/              -> generated output compiled by firmware; never edit directly
eez/pulsmon/         -> EEZ Studio project and saved app state; edit only through EEZ Studio
```

Runtime integration belongs in non-generated modules such as `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` and service/client files.

## Maintenance

- keep network I/O outside LVGL rendering;
- reject invalid JSON without clearing the last valid cache;
- do not log secrets;
- preserve compact, stable API parsing;
- update `../docs/` and `../docs/fr/` together when behavior changes.

Do not create commits, push changes or access remote repositories without explicit instruction.
