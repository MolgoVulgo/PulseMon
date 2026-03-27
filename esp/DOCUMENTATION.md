# Documentation technique firmware ESP32-S3

## Objet
Ce dossier contient le firmware ESP32-S3 (ESP-IDF + LVGL) du projet PulseMon.

Depuis l'integration dans le depot principal:
- `platformio.ini` est a la racine du projet;
- les sources firmware restent sous `esp/`.

## Arborescence de reference

```text
PulseMon/
├── platformio.ini
└── esp/
    ├── DOCUMENTATION.md
    ├── CMakeLists.txt
    ├── boards/
    │   └── 320x480.json
    ├── libraries/
    │   └── readme.txt
    ├── scripts/
    ├── src/
    │   ├── CMakeLists.txt
    │   ├── ui_screen.c
    │   ├── ui_screen.h
    │   ├── ui_backend.h
    │   ├── lv_port.c
    │   ├── lv_port.h
    │   ├── lv_conf.h
    │   ├── esp_bsp.c
    │   ├── esp_bsp.h
    │   ├── esp_lcd_axs15231b.c
    │   ├── esp_lcd_axs15231b.h
    │   ├── esp_lcd_touch.c
    │   ├── esp_lcd_touch.h
    │   └── display.h
    ├── sdkconfig.defaults
    └── sdkconfig.LVGL-320-480
```

## Configuration PlatformIO (racine)
Le fichier `platformio.ini` racine doit pointer vers `esp/`:
- `boards_dir = esp/boards`
- `lib_dir = esp/libraries`
- `src_dir = esp/src`
- `LV_CONF_PATH=${PROJECT_DIR}/esp/src/lv_conf.h`
- `lib_deps = lvgl/lvgl@^8.4.0` (telechargement automatique)

LVGL n'est pas versionne dans ce depot (`esp/libraries/lvgl-v8` ignore).
La bibliotheque est telechargee par PlatformIO au build si absente localement.

## Roles des modules
- `esp/src/esp_bsp.*`: initialisation materiel carte/ecran/tactile/backlight.
- `esp/src/lv_port.*`: integration LVGL (tick, buffers, flush, input).
- `esp/src/esp_lcd_*.*`: pilotes LCD/tactile bas niveau.
- `esp/src/ui_screen.*`: logique d'ecran principale.
- `esp/src/ui_backend.h`: interface de liaison donnees backend -> UI.
- `esp/src/lv_conf.h`: configuration compile-time LVGL.

## Build firmware
Depuis la racine du depot:

```bash
pio run -e LVGL-320-480
```

Flash/monitor selon cible:

```bash
pio run -e LVGL-320-480 -t upload
pio device monitor
```

## Regles de maintenance
- Ne pas reintroduire de sous-repertoire Git dans `esp/` (`.git` interdit).
- Conserver la separation reseau/parsing/cache/UI cote firmware.
- Garder `platformio.ini` a la racine pour une compilation unique du projet.
