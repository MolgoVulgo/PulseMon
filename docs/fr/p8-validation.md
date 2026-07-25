# Validation P8 complète par Codex

P8 s’exécute sur la station de développement avec PlatformIO, l’ESP32-S3 connecté et le backend PulseMon accessible à l’adresse locale suivante :

```text
http://192.168.0.10:8000
```

Le runner `tools/p8_validate.py` produit des preuves JSON et Markdown. Le profil `--codex-full` constitue l’autorisation explicite de cette phase pour les opérations suivantes : tests backend, builds release/debug, flash release, capture série bornée et contrôles API.

## Prérequis

- Python 3.11 ou supérieur ;
- dépendances backend déjà disponibles dans `api/.venv` ou l’environnement Python actif ;
- PlatformIO Core disponible via `pio` ;
- ESP32-S3 connecté à un port série visible ;
- backend PulseMon lancé et accessible sur `192.168.0.10:8000` ;
- accès physique à l’écran pour les observations finales.

Le runner n’installe aucune dépendance et ne consulte aucun dépôt distant.

## Étape 1 — exécution automatisée complète

Depuis la racine du dépôt :

```bash
python3 tools/p8_validate.py --codex-full
```

Le profil réalise obligatoirement :

1. le précontrôle de configuration backend ;
2. la suite pytest complète ;
3. le build `pulsmon-esp32s3-display` ;
4. le build `pulsmon-esp32s3-display-dev` ;
5. la détection du port série ;
6. le flash de l’environnement release ;
7. une capture série de 180 secondes ;
8. les contrôles HTTP sur :
   - `/api/v1/health` ;
   - `/api/v1/dashboard` ;
   - `/api/v1/gpu/dashboard` ;
9. la génération de `report.json`, `report.md` et `hardware-checklist.json`.

Si un seul port série est visible, il est sélectionné automatiquement. Avec plusieurs ports, Codex doit identifier le port de l’ESP32-S3 et relancer :

```bash
python3 tools/p8_validate.py --codex-full --port /dev/ttyACM0
```

L’adresse backend peut être remplacée uniquement si l’environnement local diffère :

```bash
python3 tools/p8_validate.py \
  --codex-full \
  --backend-url http://192.168.0.10:8000
```

Le premier rapport reste `partial` tant que les observations matérielles ne sont pas renseignées. Ce statut est attendu et ne constitue pas une validation finale.

## Étape 2 — observations matérielles obligatoires

Codex doit guider l’opérateur pour vérifier chaque point et inscrire une preuve concise dans le fichier `hardware-checklist.json` généré :

- métriques CPU, mémoire et GPU visibles ;
- changement d’hôte/port backend appliqué sans redémarrage ;
- appui cinq secondes dans le coin supérieur gauche ouvrant `PulseMon-Setup` ;
- maintien de la connexion station pendant la fenêtre AP ;
- portail non exposé comme service permanent sur le LAN station ;
- fermeture de l’AP manuel après environ dix minutes ;
- conservation du dernier état et récupération après perte backend ;
- récupération après perte Wi-Fi sans effacement NVS ;
- persistance de la configuration après redémarrage ;
- actualisation OpenWeather et GNews via TLS.

Les statuts autorisés sont `pass`, `fail`, `skip` et `pending`. Une validation P8 complète exige `pass` sur les dix contrôles.

## Étape 3 — finalisation du même rapport

Après mise à jour de la checklist, Codex doit fusionner les observations avec les preuves automatisées existantes :

```bash
python3 tools/p8_validate.py \
  --resume-report .pulsemon-results/p8/<execution>/report.json \
  --checklist-file .pulsemon-results/p8/<execution>/hardware-checklist.json \
  --require-hardware
```

`--resume-report` conserve les résultats des tests, builds, flash, monitor et contrôles API. Il ne les remplace pas par un rapport limité à la checklist.

## Critère de validation

P8 est validée uniquement si le rapport final indique :

```text
overall_status = pass
```

Codex doit rapporter :

- la commande exacte ;
- le port série utilisé ;
- le chemin de `report.json` et `report.md` ;
- les builds release/debug ;
- le résultat du flash ;
- la durée de capture série ;
- les trois contrôles API ;
- les dix observations matérielles ;
- tout extrait de log utile en cas d’échec.

Un build réussi seul, un flash réussi seul ou une checklist incomplète ne vaut pas validation P8.
