# Développement

## Installation et tests backend

```bash
cd api
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/pytest -q
```

Lancement backend :

```bash
.venv/bin/python -m uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Tests E2E de l’UI backend :

```bash
cd api
npm ci
npx playwright install chromium
npm run test:e2e
```

Ne pas installer automatiquement des dépendances pendant un correctif sauf nécessité du périmètre et autorisation de l’environnement.

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

## Workflow EEZ

- Ne jamais modifier directement `esp/src/ui/`.
- Ne jamais modifier le projet et l’état sauvegardé sous `esp/eez/pulsmon/` hors EEZ Studio.
- Appliquer les changements de design dans EEZ Studio, régénérer, puis intégrer le runtime via les modules non générés.

## Périmètre FAN

FAN est conservé mais inactif. Une modification documentaire ou de maintenance ne doit pas réactiver implicitement l’écran, le polling ou la navigation FAN.

## Génération du snapshot

Depuis la racine :

```bash
./make-a.sh
```

Le script recrée `PulseMon.zip` et exclut volontairement les contenus locaux ou de travail : `.git`, environnements virtuels, builds PlatformIO, caches, `tmp/`, `sdkconfig*` locaux, modules Node et rapports de tests.

L’absence d’un fichier dans `PulseMon.zip` ne suffit donc pas à conclure qu’il manque dans le worktree développeur. Le snapshot est volontairement filtré.

## Baseline documentaire

Avant de documenter un comportement :

1. vérifier le chemin de code actif ;
2. vérifier le fichier de configuration ou la variable réellement utilisée ;
3. distinguer composants actifs, conservés et abandonnés ;
4. mettre à jour ensemble les versions anglaise et française ;
5. ne pas présenter une cible future comme déjà implémentée.
