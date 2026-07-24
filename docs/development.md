# Development

## Backend setup and tests

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pytest -q
```

Run the backend:

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Backend UI E2E tests:

```bash
cd api
npm ci
npx playwright install chromium
npm run test:e2e
```

Do not install dependencies automatically during patch work unless the task requires it and the environment authorizes it.

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

## EEZ workflow

- Never edit `esp/src/ui/` directly.
- Never edit the EEZ project and saved state under `esp/eez/pulsmon/` outside EEZ Studio.
- Apply UI design changes in EEZ Studio, regenerate, then integrate runtime behavior through non-generated modules.

## FAN scope

FAN is retained but inactive. Documentation or maintenance changes must not silently reactivate the firmware FAN screen, polling or navigation.

## Snapshot generation

From the project root:

```bash
./make-a.sh
```

The script recreates `PulseMon.zip` and intentionally excludes local or generated working content including `.git`, virtual environments, PlatformIO build directories, caches, `tmp/`, local `sdkconfig*`, Node modules and test reports.

Absence from `PulseMon.zip` is not sufficient to classify a file as missing from the developer worktree. The snapshot is intentionally filtered.

## Documentation baseline

Before documenting behavior:

1. verify the active code path;
2. verify the actual configuration file or environment variable;
3. distinguish active, retained and abandoned components;
4. update English and French files together;
5. avoid describing planned behavior as implemented.
