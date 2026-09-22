# PulseMon agent context

## Source of truth

Use the connected Google Drive publication under `pulsemon/` as the project context source. Read `pulsemon/REPO_INDEX.json` first and use it as the map of the current published source set. For every file involved in a task, verify that the path is declared in the index and read the actual Drive file content before relying on it.

`pulsemon/patch/` is the authorized delivery area for PulseMon patch archives. It is not project source, is not part of the indexed baseline and must never be consumed implicitly as an already-applied patch chain. When the user explicitly asks to apply the latest patch, list `gdrive:pulsemon/patch/` first, select the latest applicable `patch_XXXX.zip` (or highest correction suffix for that patch when applicable), and use that archive as the patch input. `PulseMon.zip` may still exist as a local or explicitly supplied export, but it is not the default source of truth unless the user explicitly designates a specific ZIP as the working base.

`diagnostics/` is evidence only. Files under `diagnostics/patch_XXXX/` may describe the result of applying or validating a patch, but they are never patch archives, never an applicable diff source and never a substitute for `pulsemon/patch/`.

The connected PulseMon Google Drive is the authorized remote source for this workflow. Do not use GitHub or another remote repository as a fallback, and do not reconstruct intentionally filtered files from assumptions. A local instruction file that clearly belongs to another project, such as an ElegooSlicer-specific rules file, does not override the PulseMon source model unless the user explicitly makes it applicable to PulseMon.

Read first:

1. `REPO_INDEX.json`;
2. `README.md` or `README.fr.md`;
3. `docs/README.md` and the relevant canonical document;
4. `api/app/main.py` for active backend routes;
5. `api/app/config.py` for the complete 20-variable `STATS_*` contract;
6. `esp/platformio.ini` for firmware environments;
7. `esp/src/main.c`, `pulsemon_api_settings.*`, `pulsemon_api_config.h`, `pulsemon_poller.c` and the relevant firmware modules;
8. the exact files involved in the requested change.

## Active architecture

```text
Linux backend: api/
ESP32-S3 firmware: esp/
Canonical documentation: docs/ and docs/fr/
```

Backend entry point: `api/app/main.py`.
Firmware entry point: `esp/src/main.c`.

## Current product contract

- monitoring: CPU, memory and AMD GPU, including GPU fan telemetry when exposed by the driver;
- backend state: current snapshots and bounded histories in memory only;
- backend HTTP API: seven monitoring routes under `/api/v1` plus `/ui`;
- firmware screens: Main, GPU and Weather, plus Printer only while the configured printer is available;
- external content: OpenWeather and GNews fetched directly by the ESP32-S3;
- printer telemetry: direct read-only LAN HTTP/MQTT client in `esp/src/printer_service.*`, with temporary compile-time endpoint/credential placeholders in `esp/src/printer_config.h`;
- firmware persistence: backend host/port, Wi-Fi, weather and news settings in NVS;
- backend endpoint: NVS namespace `pulsemon_api`, with compiled fallback in `esp/src/pulsemon_api_config.h`;
- backend API key: optional server capability not currently sent by the firmware or backend UI;
- configuration portal: HTTP server and captive DNS active only while the setup AP is active, with DNS bound to `192.168.4.1`;
- local trigger: five-second hold in the top-left corner of Main/GPU/Weather, opening a ten-minute manual AP window while preserving station connectivity;
- printer navigation: Weather left -> Printer only when `printer_service_is_available()` is true; Printer right -> Weather; if printer availability is lost while displayed, return to Weather.

There is no active backend FAN subsystem, database layer, user-configuration API or administration UI. Do not reintroduce these contracts implicitly.

## Generated UI boundary

- Never edit `esp/src/ui/` directly.
- Never edit `esp/eez/pulsmon/` outside EEZ Studio.
- Runtime integration changes belong in the owning non-generated modules. Printer integration specifically belongs in `printer_service.*`, `printer_config.h`, `actions.c`, `ui_screen.c` and runtime variable code, never in `esp/src/ui/`.

The generated output retains an unreachable legacy FAN screen and compatibility bindings. They are not an active feature. Their physical removal requires EEZ Studio and regeneration.

## Transport and secrets

- OpenWeather and GNews use HTTPS with certificate validation through the ESP-IDF certificate bundle.
- GNews uses `X-Api-Key`.
- Do not log or return Wi-Fi passwords, API keys or printer access codes.
- Printer communication is read-only; do not add start/pause/resume/stop, heater, fan, upload or other printer-control commands without explicit scope.
- Treat the deployment as local and personal, but avoid unnecessary LAN exposure.
- Do not make the configuration portal or captive DNS permanent station-LAN services.

## Publication rules

`sync-drive.sh` publishes a deliberately filtered project view to Google Drive and generates `REPO_INDEX.json` from the same filter rules. Files excluded by `sync-drive.filter` may exist in the developer worktree without being part of the published context. Do not classify an intentionally excluded file as missing unless the active contract requires it to be published.

`patch/` is excluded from source synchronization and indexing so delivered patch archives remain separate from the project baseline. A newer `REPO_INDEX.json` publication becomes the current context baseline; historical patch archives do not modify that baseline by themselves. The exception is an explicit local application request such as "apply the latest patch": in that case, `gdrive:pulsemon/patch/` is the authorized source from which the patch archive is selected, while `REPO_INDEX.json` remains the baseline map.

`diagnostics/` remains separate from `patch/`. Diagnostic folders are outputs/evidence tied to a patch number and must never be selected as an archive to apply. Human-readable diagnostic reports for PulseMon are written in French only; do not duplicate the same diagnostic section in English. Machine-readable JSON may keep stable field names.

## Validation rules

Backend:

```bash
cd api
python3 -m app.config
python3 -m pytest -q
```

All backend environment variables are validated centrally before runtime startup. Do not add direct `STATS_*` reads elsewhere in the Python runtime; the launcher may read validated bind/port/logging values to build the Uvicorn command.

Firmware build, flash or monitor must not be run without explicit user instruction. An instruction to apply the latest patch alone does not authorize a firmware build. A build is authorized only when the user explicitly asks to build/compile/validate the firmware, or explicitly asks to reproduce/investigate a compilation failure by compiling. Merely mentioning that diagnostics should be updated if a compilation error exists does not by itself authorize a build. When a build is requested, use an environment defined in `esp/platformio.ini`.

For the explicitly authorized P8 phase, Codex must run `python3 tools/p8_validate.py --codex-full` from the project root. This profile tests the backend, builds both firmware environments, flashes release firmware, captures 180 seconds of serial output and checks `http://192.168.0.10:8000`. Preserve `report.json`, `report.md` and `hardware-checklist.json`, then finalize the same evidence with `--resume-report ... --checklist-file ... --require-hardware`. Never claim hardware validation while any manual check is not `pass`.

Documentation changes must keep English and French aligned and must describe current behavior rather than intended future behavior.

Do not create commits, push changes or access remote repositories without explicit instruction.
