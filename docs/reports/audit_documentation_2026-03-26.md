# Audit documentation - 2026-03-26

## Constat technique

Perimetre audite:
- ancien dossier `Docs/`
- coherence avec le backend `api/` et le contrat `api/docs/API_CONTRACT_V1.md`

Ecarts detectes:
1. Chemins d'API incoherents dans la spec ESP32 (`/api/stats`, `/api/stats/history`) alors que le code expose `/api/v1/dashboard` et `/api/v1/history`.
2. Exemples JSON dashboard/history de la spec ESP32 non alignes avec la structure reelle (`mem.*`, `series.cpu_pct`, `window_s`, `step_s`, `ts_ms`).
3. Gouvernance documentaire contradictoire: la spec ESP32 se declarait "source unique" et invalidait les cahiers/plans encore utilises comme references du projet.
4. Documentation eclatee entre `Docs/` et `docs/`.

## Actions appliquees

1. Redecoupage et migration des documents de `Docs/` vers `docs/`:
- `docs/specs/`
- `docs/plans/`
- `docs/adr/`
- `docs/reports/`

2. Normalisation des points d'entree documentation:
- ajout de `docs/README.md`
- ajout de `Docs/README.md` (redirection)

3. Correction de coherence de la spec ESP32:
- endpoints corriges vers `/api/v1/*`
- mapping JSON aligne sur `api/docs/API_CONTRACT_V1.md`
- retrait des assertions de gouvernance incompatibles

4. Mise a jour ADR UI:
- references vers nouveaux chemins `docs/...`
- mention explicite du contrat API backend

## Validation executee

- verification des endpoints reels dans `api/app/main.py`
- verification du contrat V1 dans `api/docs/API_CONTRACT_V1.md`
- verification de l'arborescence finale `docs/`
- verification des references texte sur mots-cles (`/api/stats`, `spec_esp32`, `cahier_*`)

## Limites / hypotheses

- L'audit est documentaire (pas de changement runtime backend/firmware).
- La coherence fonctionnelle firmware reste dependante du code ESP32 en cours d'implementation.
