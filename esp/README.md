# Documentation technique firmware ESP32-S3

## Objet
Ce dossier contient le firmware ESP32-S3 (ESP-IDF + LVGL) du projet PulseMon.

## Arborescence de reference

```text
esp/
├── platformio.ini
├── README.md
├── CMakeLists.txt
├── boards/
│   └── 320x480.json
├── libraries/
│   └── readme.txt
├── src/
│   ├── main.c
│   ├── pulsemon_api_client.c
│   ├── pulsemon_poller.c
│   ├── ui_screen.c
│   ├── ui_graphs.c
│   ├── lv_port.c
│   ├── esp_bsp.c
│   ├── esp_lcd_axs15231b.c
│   ├── esp_lcd_touch.c
│   └── ui/ (genere)
├── sdkconfig.defaults
└── sdkconfig.LVGL-320-480
```

## Configuration PlatformIO
Le fichier `platformio.ini` local configure:
- `boards_dir = boards`
- `lib_dir = libraries`
- `src_dir = src`
- `LV_CONF_PATH=${PROJECT_DIR}/src/lv_conf.h`
- `lib_deps = lvgl/lvgl@^8.4.0`

## Roles des modules
- `src/main.c`: demarrage systeme, Wi-Fi STA, initialisation UI, lancement poller.
- `src/pulsemon_api_client.*`: client HTTP `GET /api/v1/dashboard` + parsing JSON.
- `src/pulsemon_poller.*`: polling periodique backend et mise a jour UI.
- `src/ui_screen.*`: logique ecran principal (bindings vars UI).
- `src/ui_graphs.*`: gestion des graphes LVGL.
- `src/esp_bsp.*`, `src/esp_lcd_*.*`: BSP, ecran et tactile.
- `src/lv_port.*`: integration LVGL (tick, buffers, flush, input).
- `src/ui/`: fichiers UI generes (pas de modification manuelle).

## Build firmware
Depuis `esp/`:

```bash
pio run -e LVGL-320-480
```

## Regles de maintenance
- Ne pas modifier directement `src/ui/` (fichiers generes).
- Conserver la separation reseau / parsing / cache / UI.
- Limiter les allocations dynamiques et les blocages longs dans les taches critiques.
