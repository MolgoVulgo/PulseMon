# Audit documentation - 2026-03-26

## Constat technique

Perimetre audite:
- coherence entre `docs/specs/`, `docs/plans/` et le backend `api/`
- alignement avec le contrat `api/docs/API_CONTRACT_V1.md`

Ecarts detectes:
1. Endpoints API non alignes entre documents et implementation backend.
2. Exemples JSON dashboard/history non alignes avec la structure reelle (`mem.*`, `series.cpu_pct`, `window_s`, `step_s`, `ts_ms`).
3. Regles de gouvernance documentaire contradictoires entre spec et documents normatifs actifs.
4. Points d'entree documentaires insuffisamment explicites pour les references normatives.

## Actions appliquees

1. Normalisation de l'arborescence documentaire active:
- `docs/specs/`
- `docs/plans/`
- `docs/adr/`
- `docs/reports/`

2. Normalisation des points d'entree documentation:
- ajout de `docs/README.md`

3. Correction de coherence de la spec ESP32:
- endpoints corriges vers `/api/v1/*`
- mapping JSON aligne sur `api/docs/API_CONTRACT_V1.md`
- retrait des assertions de gouvernance incompatibles

4. Mise a jour ADR UI:
- references vers chemins `docs/...`
- mention explicite du contrat API backend

## Validation executee

- verification des endpoints reels dans `api/app/main.py`
- verification du contrat V1 dans `api/docs/API_CONTRACT_V1.md`
- verification de l'arborescence finale `docs/`
- verification des references texte sur mots-cles (`spec_esp32`, `cahier_*`, `API_CONTRACT_V1`)

## Limites / hypotheses

- L'audit est documentaire (pas de changement runtime backend/firmware).
- La coherence fonctionnelle firmware reste dependante du code ESP32 en cours d'implementation.
