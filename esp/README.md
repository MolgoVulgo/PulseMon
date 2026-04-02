# Firmware ESP32-S3 (PulseMon)

Firmware ESP-IDF + LVGL du projet PulseMon.

## Objectif

Afficher les metriques backend sur ecran local:
- page `Main`: CPU / RAM / GPU (snapshot global)
- page `GPU`: details GPU AMD

## Arborescence utile

```text
esp/
├── platformio.ini
├── boards/320x480.json
├── sdkconfig.defaults
└── src/
    ├── main.c
    ├── pulsemon_api_client.c
    ├── pulsemon_poller.c
    ├── ui_screen.c
    ├── ui_graphs.c
    ├── vars.c
    └── ui/   (genere, ne pas modifier)
```

## Endpoints consommes actuellement

Le firmware consomme en production:
- `GET /api/v1/dashboard`
- `GET /api/v1/gpu/dashboard`

Important:
- le code ne consomme pas encore `/api/v1/history` ni `/api/v1/gpu/history`;
- les graphes LVGL sont alimentes localement (echantillonnage des variables affichees a 1 Hz).

## Comportement runtime

- Poller: `PULSEMON_DASHBOARD_POLL_MS` (defaut `1000` ms)
- Si ecran actif = `Main`: fetch `/dashboard`
- Si ecran actif = `GPU`: fetch `/gpu/dashboard`
- En echec fetch: message `backend offline` et conservation implicite des dernieres valeurs UI
- Parsing JSON tolerant:
  - priorite `value_display`
  - fallback `value_raw`
  - fallback scalaire direct si necessaire

## Configuration API firmware

Definie dans `src/pulsemon_api_config.h`:
- `PULSEMON_API_HOST`
- `PULSEMON_API_PORT`
- `PULSEMON_API_BASE_URL`
- `PULSEMON_HTTP_TIMEOUT_MS`
- `PULSEMON_DASHBOARD_POLL_MS`

La valeur par defaut est une URL LAN statique (`192.168.0.10`).

## Build

```bash
cd esp
pio run -e LVGL-320-480
```

## Regles de maintenance

- Ne jamais modifier `src/ui/` directement (fichiers generes).
- Preserver le decouplage: reseau -> parsing -> vars -> rendu UI.
- Eviter les allocations dynamiques non necessaires dans le chemin nominal.

## Debug latence

Flags de build (dans `platformio.ini`):
- `PULSEMON_LATENCY_DEBUG`
- `PULSEMON_UI_WARN_MS`
- `PULSEMON_HTTP_WARN_MS`

## Capture LCD periodique (debug)

Si `PULSEMON_SCREENSHOT_DEBUG=1`:
- capture framebuffer RGB565 periodique;
- ecriture BMP sur SD (`PULSEMON_SCREENSHOT_DIR`);
- intervalle: `PULSEMON_SCREENSHOT_INTERVAL_MS`.
