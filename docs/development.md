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

## Optional local snapshot generation

From the project root:

```bash
./make-a.sh
```

The script recreates the filtered local export `PulseMon.zip`. It remains useful for workflows that explicitly request a ZIP, but it is not the default patch-analysis baseline when the connected Google Drive publication is available. Absence from this ZIP is not sufficient to classify a file as missing from the developer worktree or from the Drive publication.

## Patch baseline

The connected Google Drive `pulsemon/` publication is the default context baseline. Read `REPO_INDEX.json` first, record its `generated_at` and `file_count`, verify that every targeted source path is declared there, then read the actual Drive content before analysis or modification. A more recent published `REPO_INDEX.json` replaces the previous baseline.

`pulsemon/patch/` is only the patch delivery area. Inspect it to choose the next available patch number and to confirm delivery, but never treat archives found there as already applied source unless the user explicitly says so. `PulseMon.zip` is a baseline only when the user explicitly designates a specific ZIP for that task. GitHub and other remote repositories are not fallback sources.

## Documentation baseline

Before documenting behavior:

1. verify the active code path;
2. verify the actual configuration file or environment variable;
3. distinguish active runtime, generated compatibility artifacts and planned changes;
4. update English and French files together;
5. avoid describing planned behavior as implemented.

## Google Drive synchronization

Before running `./sync-drive.sh`, Codex reviews the files for secrets, runs the applicable tests and prepares or clears diagnostics. The script only generates `REPO_INDEX.json`, runs `rclone sync` to `${REMOTE:-gdrive:pulsemon}` and publishes the index after success. It does not run tests or produce diagnostics.

`sync-drive.filter` is shared by the local rclone listing used for the index and the transfer. The index excludes itself. Active sources, documentation, build configuration, required generated UI, EEZ project, packaging and selected tests are included. Local environments, builds, caches, secrets/environment files, `tmp/`, `tools/`, saved EEZ editor state, old captures, local editor/agent settings and the root `patch/` delivery area are excluded. The historical removed-route test and the tests depending on excluded P8 tooling are excluded from this publication. The required generated legacy screen remains an inactive compatibility artifact.

Keep intentionally prepared external-analysis evidence under root `diagnostics/`; `.gitkeep` preserves the directory when empty. `api/diagnostics/` contains runtime captures and is excluded. Patch ZIPs are uploaded separately to `pulsemon/patch/` after packaging validation and must be confirmed by Drive read-back or folder relisting. The script does not scan file contents for secrets; that review remains a prerequisite. `rclone sync` mirrors included source files while the excluded `patch/` area remains outside that synchronization.
