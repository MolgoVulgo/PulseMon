# Development

## Backend setup and tests

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pytest -q
```

Validate backend configuration:

```bash
cd api
python3 -m app.config
```

Run the backend:

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Do not install dependencies automatically during controlled patch work unless the task requires it and the environment authorizes it.

## Firmware build

```bash
cd esp
pio run -e pulsmon-esp32s3-display
```

Development build:

```bash
cd esp
pio run -e pulsmon-esp32s3-display-dev
```

Build, flash and monitor operations require an explicit request during controlled patch work.

## Reproducible P8 validation

After explicit P8 authorization, Codex must run the complete profile:

```bash
python3 tools/p8_validate.py --codex-full
```

The profile runs the backend precheck and tests, builds both PlatformIO environments, flashes the release environment, captures 180 seconds of serial output and checks the backend at `http://192.168.0.10:8000`. The same report must then be finalized from the hardware checklist with `--resume-report` and `--require-hardware`. See `p8-validation.md`.

## EEZ workflow

- Never edit `esp/src/ui/` directly.
- Never edit the EEZ project or saved state under `esp/eez/pulsmon/` outside EEZ Studio.
- Apply UI design changes in EEZ Studio, regenerate, then integrate runtime behavior through non-generated modules.

The generated project currently retains an unreachable legacy FAN screen. Removing it is an EEZ Studio task, not a direct source-edit task.

## Snapshot generation

From the project root:

```bash
./make-a.sh
```

The script recreates `PulseMon.zip` and intentionally excludes local working content including `.git`, virtual environments, PlatformIO build directories, caches, `tmp/`, local `sdkconfig*`, Node modules and test reports.

Absence from `PulseMon.zip` is not sufficient to classify a file as missing from the developer worktree. The snapshot is intentionally filtered.

## Patch baseline

A newly supplied `PulseMon.zip` replaces the previous snapshot and its patch chain. Patch analysis and delivery must start from the new snapshot only. Remote repositories are not a fallback source.

## Documentation baseline

Before documenting behavior:

1. verify the active code path;
2. verify the actual configuration file or environment variable;
3. distinguish active runtime, generated compatibility artifacts and planned changes;
4. update English and French files together;
5. avoid describing planned behavior as implemented.
