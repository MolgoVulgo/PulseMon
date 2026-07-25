# Développement

## Installation et tests backend

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pytest -q
```

Valider la configuration backend :

```bash
cd api
python3 -m app.config
```

Lancement backend :

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Ne pas installer automatiquement de dépendances pendant un correctif contrôlé sauf nécessité du périmètre et autorisation de l’environnement.

## Build firmware

```bash
cd esp
pio run -e pulsmon-esp32s3-display
```

Build développement :

```bash
cd esp
pio run -e pulsmon-esp32s3-display-dev
```

Les opérations de build, flash et monitor nécessitent une demande explicite pendant un travail de correctif contrôlé.

## Validation P8 reproductible

Après autorisation explicite de P8, Codex doit exécuter le profil complet :

```bash
python3 tools/p8_validate.py --codex-full
```

Ce profil réalise le précontrôle et les tests backend, compile les deux environnements PlatformIO, flashe l’environnement release, capture 180 secondes de série et vérifie le backend sur `http://192.168.0.10:8000`. Le même rapport doit ensuite être finalisé depuis la checklist matérielle avec `--resume-report` et `--require-hardware`. Voir `p8-validation.md`.

## Workflow EEZ

- Ne jamais modifier directement `esp/src/ui/`.
- Ne jamais modifier le projet ou l’état sauvegardé sous `esp/eez/pulsmon/` hors EEZ Studio.
- Appliquer les changements de design dans EEZ Studio, régénérer, puis intégrer le runtime via les modules non générés.

Le projet généré conserve actuellement un ancien écran FAN inaccessible. Son retrait est une tâche EEZ Studio, pas une édition directe des sources.

## Génération du snapshot

Depuis la racine :

```bash
./make-a.sh
```

Le script recrée `PulseMon.zip` et exclut volontairement les contenus de travail locaux : `.git`, environnements virtuels, builds PlatformIO, caches, `tmp/`, `sdkconfig*` locaux, modules Node et rapports de tests.

L’absence d’un fichier dans `PulseMon.zip` ne suffit pas à conclure qu’il manque dans le worktree développeur. Le snapshot est volontairement filtré.

## Baseline de patch

Un nouveau `PulseMon.zip` remplace le snapshot précédent et sa chaîne de patchs. L’analyse et la livraison doivent repartir uniquement de ce nouveau snapshot. Un dépôt distant n’est pas une source de secours.

## Baseline documentaire

Avant de documenter un comportement :

1. vérifier le chemin de code actif ;
2. vérifier le fichier de configuration ou la variable d’environnement réelle ;
3. distinguer runtime actif, artefacts générés de compatibilité et changements planifiés ;
4. mettre à jour anglais et français ensemble ;
5. ne pas présenter un comportement planifié comme implémenté.
