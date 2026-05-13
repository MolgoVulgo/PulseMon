# Development

## Backend setup

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Backend run

```bash
cd api
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Backend tests

```bash
cd api
.venv/bin/pytest -q
```

## UI tests

If the backend UI test suite is present:

```bash
npm install
npx playwright install chromium
npm run test:e2e
```

Covered behavior should include UI non-regression around fan configuration selection and polling.

## Firmware build

```bash
cd esp
pio run -e LVGL-320-480
```

## Maintenance rules

- Keep backend sampling separated from HTTP handlers.
- Keep firmware HTTP polling separated from LVGL rendering.
- Keep API payloads compact and stable.
- Keep metric units fixed.
- Keep unavailable metrics explicit and nullable or invalid.
- Do not log API keys.
- Do not put API keys in normal request URLs when a header is available.
- Do not edit generated ESP32 UI files directly.
- Update English and French documentation together.

## Contract changes

Any API contract change must include:

- updated documentation;
- backend tests;
- firmware parsing impact review;
- compatibility behavior for existing fields where possible.
