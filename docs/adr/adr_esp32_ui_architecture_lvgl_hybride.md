# adr_esp32_ui_architecture_lvgl_hybride.md

# ADR — Architecture UI ESP32 LVGL hybride

## Statut

**Actif — non normatif pour le contrat JSON**

Ce document décrit l’architecture firmware retenue.
Il **ne définit pas** le contrat API.
La référence d’intégration est :

- `docs/specs/spec_esp32_integration_stats_linux.md`
- `api/docs/API_CONTRACT_V1.md` pour le contrat HTTP réellement exposé

---

## 1. Décision

L’interface ESP32 repose sur une approche **LVGL hybride** :

- LVGL pour l’ossature UI
- composants custom pour le rendu critique
- moteur de graphe spécifique hors `lv_chart`

---

## 2. Pourquoi cette décision

### Constats

- les graphes temps réel sont sensibles aux micro-variations de cadence
- l’effet visuel perçu est prioritaire (pas de “sauts”)
- les métriques CPU/GPU demandent une lecture fluide et crédible
- `lv_chart` est pratique mais pas assez fin pour ce cas

### Conclusion

- **LVGL reste l’ossature**
- **le graphe est spécialisé**
- **l’animation visuelle est pilotée localement**

---

## 3. Principes structurants

- séparation stricte réseau / rendu
- séparation stricte data / display
- aucune dépendance UI directe à la latence HTTP
- aucun redraw complet imposé par un tick réseau
- pas de recalcul métier backend dans l’UI

---

## 4. Modules recommandés

- `app_main`
- `network_client`
- `json_parser`
- `data_store`
- `ui_dashboard`
- `ui_graphs`
- `graph_engine`
- `state_manager`
- `scheduler`

---

## 5. Règles de rendu

- cartes : animation légère, non bloquante
- graphes : translation fluide / redraw maîtrisé
- interpolation visuelle autorisée entre points valides
- aucune interpolation sur données invalides
- échelles stables, pas de rescale agressif

---

## 6. Rôle de LVGL

LVGL gère :

- layout global
- conteneurs
- labels
- cartes
- thèmes
- états visuels
- input éventuel
- overlay / debug

LVGL ne doit pas être le point faible des graphes.

---

## 7. Rôle du moteur de graphe custom

Le moteur custom gère :

- stockage des points affichables
- transformation en coordonnées écran
- découplage temps logique / temps rendu
- interpolation visuelle
- clipping
- invalidation minimale de zone
- continuité visuelle

---

## 8. Contraintes de performance

Objectifs :

- UI stable et lisible
- sensation de continuité
- absence de stutter perceptible
- coût CPU maîtrisé
- consommation RAM contrôlée
- aucune saturation liée au réseau

---

## 9. Règle de gouvernance

Toute évolution d’architecture UI peut être documentée ici.

Toute évolution du contrat API doit être documentée dans :

- `docs/specs/spec_esp32_integration_stats_linux.md`
- `api/docs/API_CONTRACT_V1.md`

Fin.
