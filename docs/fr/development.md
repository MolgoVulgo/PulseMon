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

Les opérations de build, flash et monitor nécessitent une demande explicite pendant un travail de correctif contrôlé. L’environnement dev définit `PULSEMON_DEBUG=1` et émet les diagnostics PulseMon de heap/DMA/stack utilisés pour la validation firmware ; l’environnement release n’active pas ces diagnostics. Les deux environnements utilisent la même politique TLS/PSRAM versionnée dans les fichiers sdkconfig du projet.

Pour une stabilisation firmware liée à la mémoire, valider d’abord l’image dev avec une capture série bornée après reset complet. Considérer `alloc_fail`, `esp-aes`, `ESP_ERR_HTTP_FETCH_HEADER`, stack overflow/canary/watchpoint, Guru Meditation, panic ou abort comme des échecs. Ne compiler l’environnement release qu’après un run dev propre.

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

## Génération optionnelle du snapshot local

Depuis la racine :

```bash
./make-a.sh
```

Le script recrée l’export local filtré `PulseMon.zip`. Il reste utile pour les workflows qui demandent explicitement un ZIP, mais il n’est plus la baseline par défaut des analyses de correctifs lorsque la publication Google Drive connectée est disponible. L’absence d’un fichier dans ce ZIP ne suffit pas à conclure qu’il manque dans le worktree développeur ou dans la publication Drive.

## Baseline de patch

La publication Google Drive connectée `pulsemon/` constitue la baseline de contexte par défaut. Lire d’abord `REPO_INDEX.json`, relever `generated_at` et `file_count`, vérifier que chaque chemin source ciblé y est déclaré, puis lire le contenu réel du fichier Drive avant analyse ou modification. Une publication plus récente de `REPO_INDEX.json` remplace la baseline précédente.

`pulsemon/patch/` est uniquement la zone de livraison des correctifs. La consulter pour choisir le prochain numéro disponible et confirmer la livraison, mais ne jamais considérer implicitement les archives présentes comme déjà appliquées. `PulseMon.zip` ne devient une baseline que si l’utilisateur désigne explicitement un ZIP précis pour la tâche. GitHub et les autres dépôts distants ne sont pas des sources de secours.

## Baseline documentaire

Avant de documenter un comportement :

1. vérifier le chemin de code actif ;
2. vérifier le fichier de configuration ou la variable d’environnement réelle ;
3. distinguer runtime actif, artefacts générés de compatibilité et changements planifiés ;
4. mettre à jour anglais et français ensemble ;
5. ne pas présenter un comportement planifié comme implémenté.

## Synchronisation Google Drive

Avant `./sync-drive.sh`, Codex recherche les secrets dans les fichiers, exécute les tests applicables et prépare ou nettoie les diagnostics. Le script génère uniquement `REPO_INDEX.json`, lance `rclone sync` vers `${REMOTE:-gdrive:pulsemon}` et publie l’index après réussite. Il ne lance aucun test et ne produit aucun diagnostic.

`sync-drive.filter` est commun à la liste locale rclone utilisée pour l’index et au transfert. L’index ne se référence pas lui-même. Les sources actives, la documentation, la configuration de build, l’UI générée nécessaire, le projet EEZ, le packaging et les tests retenus sont inclus. Les environnements locaux, builds, caches, secrets/fichiers d’environnement, `tmp/`, `tools/`, états de l’éditeur EEZ, anciennes captures, réglages locaux des éditeurs/agents et la zone de livraison racine `patch/` sont exclus. Le test historique des routes supprimées et les tests dépendant de l’outillage P8 exclu ne sont pas publiés. L’ancien écran généré nécessaire reste un artefact de compatibilité inactif.

Placer les éléments volontairement préparés pour une analyse externe dans `diagnostics/` à la racine ; `.gitkeep` préserve ce répertoire lorsqu’il est vide. `api/diagnostics/` contient les captures runtime et est exclu. Les ZIP de correctif sont déposés séparément dans `pulsemon/patch/` après validation du packaging et leur présence doit être confirmée par relecture Drive ou nouveau listing du dossier. Le script ne recherche pas de secrets dans le contenu : cette vérification reste un préalable. `rclone sync` reproduit les fichiers source inclus tandis que la zone `patch/`, exclue, reste hors de cette synchronisation.
