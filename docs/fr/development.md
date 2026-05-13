# Développement

## Installation backend

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

## Lancement backend

```bash
cd api
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## Tests backend

```bash
cd api
.venv/bin/pytest -q
```

## Tests UI

Si la suite de tests UI backend est présente :

```bash
npm install
npx playwright install chromium
npm run test:e2e
```

Le comportement couvert doit inclure la non-régression de sélection ventilateur dans l’UI pendant le polling.

## Build firmware

```bash
cd esp
pio run -e LVGL-320-480
```

## Règles de maintenance

- Séparer l’échantillonnage backend des handlers HTTP.
- Séparer le polling HTTP firmware du rendu LVGL.
- Garder les payloads API compacts et stables.
- Garder les unités fixes.
- Garder les métriques indisponibles explicites, nullables ou invalides.
- Ne pas loguer les clés API.
- Ne pas mettre les clés API dans les URLs normales quand un header existe.
- Ne pas modifier directement les fichiers UI générés ESP32.
- Mettre à jour ensemble la documentation anglaise et française.

## Modifications de contrat

Toute modification de contrat API doit inclure :

- documentation mise à jour ;
- tests backend ;
- analyse d’impact parsing firmware ;
- comportement de compatibilité pour les champs existants quand possible.
