# PulseMon ESP32-S3 agent context

## Source of truth

Use the supplied snapshot and the firmware files actually present under `esp/`. Read `../README.md`, `../docs/firmware.md`, `../docs/configuration.md`, `platformio.ini`, `src/main.c` and the exact module being changed.

## Build

```text
pulsmon-esp32s3-display      -> default release environment
pulsmon-esp32s3-display-dev  -> debug environment
```

Do not build, flash or monitor without explicit user instruction.

When P8 hardware validation is authorized, run `python3 tools/p8_validate.py --codex-full` from the project root. The profile builds release/debug, flashes release, captures 180 seconds of serial output and checks `192.168.0.10:8000`. Keep the generated JSON/Markdown evidence and finalize it with `--resume-report`, the completed checklist and `--require-hardware`; compilation alone is never physical validation.

## Active runtime

- active screens: Main, GPU and Weather, plus Printer only while the printer service reports the configured printer available;
- backend polling: main dashboard outside GPU screen, GPU dashboard on GPU screen;
- last valid values are preserved on failures;
- backend host and port are stored in NVS namespace `pulsemon_api`;
- compiled endpoint fallbacks remain in `src/pulsemon_api_config.h`;
- no backend API key is stored or sent;
- Wi-Fi, OpenWeather and GNews settings are stored in NVS;
- OpenWeather and GNews use HTTPS with certificate validation through the ESP-IDF certificate bundle;
- `printer_service.*` is a direct read-only LAN client; temporary `PRINTER_HOST` and `PRINTER_ACCESS_CODE` settings live in `printer_config.h`;
- printer navigation is Weather left -> Printer only while available, and Printer right -> Weather; loss of printer availability while displayed returns to Weather;
- the configuration HTTP server and captive DNS start only with the setup AP and stop with it;
- captive DNS binds only to `192.168.4.1`, and portal handlers reject requests when the AP is inactive;
- `config_mode_trigger.c` provides a five-second top-left touch hold that opens a ten-minute manual AP window without modifying generated EEZ files.

There is no active generic FAN client or backend FAN contract. GPU fan telemetry remains part of the GPU dashboard. Printer communication remains read-only; do not introduce printer-control commands implicitly.

## EEZ ownership

```text
src/ui/              -> generated output compiled by firmware; never edit directly
eez/pulsmon/         -> EEZ Studio project and saved app state; edit only through EEZ Studio
```

The generated output retains an unreachable legacy FAN screen and compatibility bindings. Do not treat them as active navigation and do not remove them manually. Physical removal requires EEZ Studio and regeneration. The generated Printer screen and its bindings are also EEZ-owned: they may be read as an integration contract but never edited manually.

Runtime integration belongs in non-generated modules such as `vars.c`, `actions.c`, `ui_screen.c`, `ui_graphs.c` and service/client files. Printer runtime integration belongs in `printer_service.*`, `printer_config.h`, navigation modules and runtime variables outside `src/ui/`.

## Maintenance

- keep network I/O outside LVGL rendering;
- reject invalid JSON without clearing the last valid cache;
- do not log secrets, including the printer access code;
- preserve compact, stable API parsing;
- preserve the AP-event-driven lifecycle of the configuration server and captive DNS;
- update `../docs/` and `../docs/fr/` together when behavior changes.

Do not create commits, push changes or access remote repositories without explicit instruction.
