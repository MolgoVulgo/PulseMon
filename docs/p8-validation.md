# Complete P8 validation by Codex

P8 runs on the development workstation with PlatformIO, the ESP32-S3 connected and the PulseMon backend reachable at:

```text
http://192.168.0.10:8000
```

The `tools/p8_validate.py` runner produces JSON and Markdown evidence. The `--codex-full` profile carries the explicit authorization for this phase to run backend tests, release/debug builds, release flashing, bounded serial capture and API checks.

## Prerequisites

- Python 3.11 or newer;
- backend dependencies already available in `api/.venv` or the active Python environment;
- PlatformIO Core available as `pio`;
- the ESP32-S3 connected through a visible serial port;
- the PulseMon backend running and reachable on `192.168.0.10:8000`;
- physical access to the display for final observations.

The runner installs no dependencies and accesses no remote repository.

## Step 1 — complete automated execution

From the repository root:

```bash
python3 tools/p8_validate.py --codex-full
```

The profile must run:

1. backend configuration precheck;
2. the complete pytest suite;
3. the `pulsmon-esp32s3-display` build;
4. the `pulsmon-esp32s3-display-dev` build;
5. serial-device detection;
6. release-environment flashing;
7. a 180-second serial capture;
8. HTTP checks for:
   - `/api/v1/health`;
   - `/api/v1/dashboard`;
   - `/api/v1/gpu/dashboard`;
9. generation of `report.json`, `report.md` and `hardware-checklist.json`.

When exactly one serial port is visible it is selected automatically. With multiple ports, Codex must identify the ESP32-S3 port and rerun:

```bash
python3 tools/p8_validate.py --codex-full --port /dev/ttyACM0
```

The backend address may be overridden only when the local environment differs:

```bash
python3 tools/p8_validate.py \
  --codex-full \
  --backend-url http://192.168.0.10:8000
```

The first report remains `partial` until the physical observations are filled in. This is expected and is not the final validation result.

## Step 2 — mandatory hardware observations

Codex must guide the operator through every item and record concise evidence in the generated `hardware-checklist.json` file:

- CPU, memory and GPU metrics are visible;
- backend host/port changes apply without reboot;
- a five-second top-left hold opens `PulseMon-Setup`;
- station connectivity remains active during the AP window;
- the portal is not a permanent station-LAN service;
- the manual AP closes after approximately five minutes;
- the last state is retained and data recovers after backend loss;
- Wi-Fi recovers without clearing NVS;
- configuration persists after reboot;
- OpenWeather and GNews refresh over TLS.

Allowed statuses are `pass`, `fail`, `skip` and `pending`. Complete P8 validation requires `pass` on all ten checks.

## Step 3 — finalize the same report

After updating the checklist, Codex must merge the observations into the existing automated evidence:

```bash
python3 tools/p8_validate.py \
  --resume-report .pulsemon-results/p8/<run>/report.json \
  --checklist-file .pulsemon-results/p8/<run>/hardware-checklist.json \
  --require-hardware
```

`--resume-report` preserves the test, build, flash, monitor and API results. It does not replace them with a checklist-only report.

## Validation criterion

P8 is validated only when the final report contains:

```text
overall_status = pass
```

Codex must report:

- the exact command;
- the selected serial port;
- the `report.json` and `report.md` paths;
- release/debug build results;
- flash result;
- serial-capture duration;
- all three API checks;
- all ten hardware observations;
- relevant log excerpts for any failure.

A successful build alone, successful flash alone or incomplete checklist is not P8 validation.
