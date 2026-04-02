# ADR - Architecture UI ESP32 LVGL

## Statut

Actif.

Ce document decrit l'architecture UI firmware actuellement implementee.
Le contrat API reste defini par:
- `api/docs/API_CONTRACT_V1.md`
- `api/docs/API_GPU_CONTRACT_V1.md`

## 1. Decision

L'UI ESP32 repose sur:
- ecrans generes (`esp/src/ui/`);
- logique d'orchestration locale (`ui_screen.c`, `actions.c`, `vars.c`);
- graphes via `lv_chart` (`ui_graphs.c`).

Il n'y a pas de moteur de graphe custom hors `lv_chart` dans l'etat actuel.

## 2. Rationale

- garder un pipeline simple et maintenable sur ESP32;
- limiter le couplage reseau/UI;
- conserver des temps de rendu stables en utilisant des buffers locaux et des updates periodiques.

## 3. Pipeline runtime

1. poller HTTP recupere un snapshot (`/dashboard` ou `/gpu/dashboard`).
2. parser JSON met a jour les variables UI (`set_var_*`).
3. timer labels (`250 ms`) applique les valeurs texte.
4. timer graphes (`1000 ms`) pousse un echantillon numerique dans `lv_chart`.

## 4. Principes de separation

- pas d'appel HTTP dans le thread de rendu LVGL;
- pas d'acces direct aux capteurs Linux cote ESP;
- fichiers `esp/src/ui/` traites comme generes (pas d'edition manuelle).

## 5. Contraintes connues

- les graphes sont locaux et ne consomment pas `/api/v1/history`;
- en perte reseau, les dernieres valeurs restent affichees et le statut passe en mode degrade;
- l'URL backend est statique par defaut (`pulsemon_api_config.h`).

## 6. Evolutions possibles

- consommation de `/api/v1/history` pour resynchronisation temporelle stricte;
- configuration backend dynamique (NVS, mDNS, provisioning);
- enrichissement UI sans modifier la frontiere reseau/parsing/rendu.
