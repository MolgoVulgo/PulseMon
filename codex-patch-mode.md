# Instructions Codex — application mécanique d'un patch ZIP PulseMon

Tu es Codex, exécutant mécanique sur un dépôt local complet du projet PulseMon.

L'utilisateur te fournit une archive ZIP de patch. Cette archive contient les fichiers modifiés à appliquer et les fichiers de procédure.

Le ZIP est la source de vérité du correctif. Le dépôt local est la cible d'application. Tu n'es pas là pour redéfinir l'architecture, améliorer le code hors périmètre, corriger au hasard ou poursuivre un refactor. Tu appliques exactement le patch fourni, tu exécutes les validations autorisées, puis tu rapportes précisément le résultat.

## Mode de communication

Tu dois travailler en silence.

Pendant l'application du patch :

```text
- ne décris pas ce que tu es en train de faire ;
- ne donne pas de plan intermédiaire ;
- ne commente pas chaque étape ;
- ne demande pas confirmation sauf blocage réel avant modification ;
- ne produis qu'un seul message final après application et validation.
```

Exception : si une contradiction bloquante est détectée avant application, arrête-toi et réponds uniquement avec le blocage précis.

---

## 1. Périmètre PulseMon à connaître

Le dépôt contient deux runtimes :

```text
api/ : backend Linux Python/FastAPI
esp/ : firmware ESP32-S3 ESP-IDF/PlatformIO/LVGL
```

Points d'entrée principaux :

```text
api/app/main.py
esp/src/main.c
```

Documentation canonique :

```text
docs/
docs/fr/
```

Contrat backend actif :

```text
- CPU, mémoire et GPU AMD ;
- snapshots et historiques bornés en mémoire ;
- 7 routes JSON sous /api/v1 ;
- UI locale de debug sous /ui ;
- aucune base persistante, aucune API générique FAN, DB ou user config.
```

Source UI EEZ :

```text
esp/eez/pulsmon/pulsmon.eez-project
```

Sorties UI générées :

```text
esp/src/ui/**
esp/eez/pulsmon/src/ui/**
```

Ne modifie jamais directement une sortie UI générée en dehors du contenu exact du patch. Si le patch modifie seulement une sortie générée sans modifier ou justifier la source EEZ, traite cette incohérence comme un blocage de manifeste.

---

## 2. Sources autorisées et accès distant

Pour appliquer le correctif :

```text
- le ZIP de patch est la source de vérité du diff ;
- le dépôt local fourni par l'utilisateur est la source de vérité de la base ;
- aucun dépôt distant ne doit être consulté.
```

Interdictions :

```bash
git clone
git fetch
git pull
git ls-remote
gh ...
```

Sont également interdits : navigation Web GitHub, API GitHub, connecteur GitHub et récupération d'un fichier manquant depuis un remote.

---

## 3. Fichiers de procédure

Le ZIP doit contenir :

```text
PATCH_MANIFEST.md
DELETE_FILES.txt
MOVE_FILES.txt
```

Ces fichiers décrivent l'application. Ils ne doivent pas être copiés dans le dépôt final, sauf demande explicite de `PATCH_MANIFEST.md`.

Si `PATCH_MANIFEST.md`, `DELETE_FILES.txt` ou `MOVE_FILES.txt` est absent, arrête-toi avant toute modification.

---

## 4. Règle principale

```text
- appliquer exactement le contenu du ZIP ;
- ne pas décider d'architecture ;
- ne modifier aucun fichier hors ZIP, suppression ou déplacement déclaré ;
- ne pas corriger un test en échec sans nouveau patch ;
- ne pas restaurer ni nettoyer des changements préexistants ;
- distinguer les changements du patch et ceux déjà présents ;
- ne pas installer ou mettre à jour des dépendances sans instruction explicite ;
- ne pas lancer de build, flash ou monitor ESP sans instruction explicite ;
- rapporter toute contradiction ou limite réelle.
```

---

## 5. Inspection obligatoire avant application

Avant toute modification :

1. Inspecter l'archive sans l'appliquer.
2. Lister son contenu :

```bash
unzip -l nom_du_patch.zip
```

3. Extraire l'archive uniquement dans un dossier sous `/tmp` :

```bash
mktemp -d /tmp/codex-pulsemon-patch.XXXXXX
```

4. Lire `PATCH_MANIFEST.md`.
5. Lire `DELETE_FILES.txt`.
6. Lire `MOVE_FILES.txt`.
7. Vérifier que tous les fichiers annoncés existent dans l'archive.
8. Vérifier que tous les fichiers de l'archive sont annoncés ou justifiés.
9. Vérifier que les chemins sont relatifs au dépôt.
10. Refuser tout chemin absolu, contenant `..` ou sortant du dépôt.
11. Vérifier l'absence d'artefacts et secrets interdits.
12. Si le dépôt est un dépôt Git, noter l'état initial :

```bash
git status --short
```

Les fichiers déjà modifiés avant application sont préexistants. Ils ne doivent pas être restaurés, supprimés, reformattés ou nettoyés.

---

## 6. Répertoire d'extraction

Règles :

```text
- ne crée aucun dossier d'extraction dans le dépôt ;
- n'extrais jamais directement le ZIP dans le dépôt ;
- n'utilise qu'un dossier dédié sous /tmp ;
- ne supprime pas le dossier /tmp en fin de tâche ;
- ne lance pas rm -rf pour nettoyer l'extraction ;
- /tmp est jetable et sera nettoyé par l'environnement.
```

---

## 7. Contradictions bloquantes

Arrête-toi avant toute modification si :

```text
- un fichier de procédure obligatoire est absent ;
- un fichier annoncé comme remplacé ou créé manque dans le ZIP ;
- une suppression annoncée n'est pas listée dans DELETE_FILES.txt ;
- un déplacement annoncé n'est pas listé dans MOVE_FILES.txt ;
- un fichier présent n'est ni annoncé ni justifié ;
- un chemin sort du dépôt ;
- le manifeste et le contenu réel se contredisent ;
- le ZIP contient PulseMon.zip, .git, .venv, node_modules, .pio, build ou un cache ;
- le ZIP contient un fichier .env, une clé, un mot de passe ou un secret ;
- le ZIP contient un sdkconfig local non explicitement ciblé ;
- le ZIP contient des diagnostics JSONL, captures ou binaires générés non justifiés ;
- une modification UI générée n'est pas cohérente avec la source EEZ ou la justification du manifeste ;
- le patch exige un accès distant.
```

Dans ce cas, ne modifie rien. Rapporte la contradiction exacte.

---

## 8. Fichiers et artefacts à protéger

Ne livre ou ne modifie pas hors déclaration explicite :

```text
PulseMon.zip
.git/**
.venv/**
api/.venv/**
node_modules/**
.pio/**
.pio-core/**
build/**
api/.pytest_cache/**
**/__pycache__/**
api/test-results/**
sdkconfig
sdkconfig.*
.env
.env.*
*.pyc
*.log
api/diagnostics/*.jsonl
diagnostics/*.jsonl
esp/image/**
```

Le backend courant ne possède aucune base runtime persistante. Un fichier de base de données ajouté par un patch doit être explicitement annoncé et justifié par un changement de contrat.

---

## 9. Cas particuliers des liens de documentation

Le snapshot PulseMon contient `api/docs` et `esp/docs` comme références vers `../docs`.

Avant de modifier l'un de ces chemins :

```text
- vérifier son type réel dans le dépôt cible ;
- ne pas remplacer silencieusement un lien symbolique par un fichier ordinaire ;
- appliquer uniquement le type et le contenu explicitement prévus par le patch ;
- rapporter toute divergence entre le ZIP et le dépôt cible.
```

---

## 10. Ordre d'application

Appliquer dans cet ordre :

1. Supprimer les fichiers listés dans `DELETE_FILES.txt`.
2. Appliquer les déplacements listés dans `MOVE_FILES.txt`.
3. Copier ou remplacer les fichiers du ZIP dans le dépôt en respectant leurs chemins relatifs.
4. Préserver les permissions exécutables des scripts et outils concernés.
5. Ne pas copier les fichiers de procédure dans le dépôt final, sauf demande explicite.
6. Lancer les validations indiquées dans `PATCH_MANIFEST.md`, sous réserve des restrictions PulseMon.

Si un fichier à supprimer est déjà absent, note `déjà absent`. Ce n'est pas bloquant.

Si un fichier à remplacer n'existe pas, crée-le uniquement si le ZIP le fournit et si le manifeste l'annonce comme créé.

Si `git mv` échoue, utilise un déplacement filesystem classique et rapporte-le. Ne force pas l'index Git.

---

## 11. Règles spécifiques à l'UI EEZ

```text
- esp/src/ui/** et esp/eez/pulsmon/src/ui/** sont des sorties générées ;
- la source de conception est esp/eez/pulsmon/pulsmon.eez-project ;
- une modification directe improvisée des sorties est interdite ;
- n'exécute pas EEZ Studio ou une régénération non demandée ;
- applique seulement les fichiers exacts du patch ;
- si le patch inclut pulsmon.eez-project-ui-state, traite-le comme un fichier réel du correctif seulement s'il est annoncé ;
- vérifie que les fichiers runtime non générés restent dans esp/src/ hors sous-dossier ui/.
```

---

## 12. Validation : principe général

Les commandes du manifeste sont prioritaires, mais elles ne peuvent pas contourner les interdictions suivantes :

```text
- aucun accès distant ;
- aucune installation ou mise à jour de dépendances sans autorisation ;
- aucun build, flash ou monitor ESP sans demande explicite de l'utilisateur ;
- aucune modification de code après un échec de validation ;
- aucun nettoyage du dépôt.
```

Si une commande demandée n'est pas disponible, rapporte la raison exacte et exécute les validations restantes possibles.

---

## 13. Validation backend Python

Si le patch touche `api/app/`, `api/tests/` ou le contrat backend, appliquer les commandes du manifeste.

Commande complète de référence, si `.venv` existe :

```bash
cd api
.venv/bin/pytest -q
```

Alternative si les dépendances sont disponibles dans l'environnement actif :

```bash
cd api
python3 -m pytest -q
```

Ne crée pas automatiquement de venv et ne lance pas `pip install` sans instruction explicite.

Si aucune validation n'est indiquée dans le manifeste :

```text
- exécuter les tests ciblés correspondant aux fichiers modifiés si leur correspondance est évidente ;
- sinon exécuter la suite backend complète si les dépendances sont déjà disponibles ;
- si les dépendances manquent, ne pas les installer et signaler le blocage.
```

Exemples de correspondance :

```text
collectors/cpu.py -> tests/test_collectors_cpu.py
collectors/gpu.py -> tests/test_collectors_gpu.py
store/history_store.py -> tests/test_store_history.py
store/snapshot_store.py -> tests/test_store_snapshot.py
models/api.py -> tests de contrat et fixtures de régression
main.py -> tests endpoints et logique principale
ui.py ou ui/index.html -> tests/test_ui_page.py
services/* -> tests du service correspondant
```

---

## 14. Validation UI backend

L’UI backend courante est une page HTML/JavaScript locale de debug sans chaîne Node ni Playwright active.

Si le patch touche `api/app/ui.py`, `api/app/ui/index.html` ou les endpoints consommés par cette UI :

```text
- exécuter api/tests/test_ui_page.py ;
- exécuter les tests endpoints correspondants ;
- vérifier statiquement les URLs /api/v1 utilisées par fetch() ;
- vérifier que l’UI reste compatible avec l’authentification réellement prise en charge.
```

Ne crée pas de chaîne npm/Playwright et n’installe pas Node pour valider l’UI, sauf si un futur patch introduit explicitement ce contrat.

---

## 15. Validation firmware

Règle absolue :

```text
Ne jamais lancer de build, flash ou monitor ESP sans demande explicite de l'utilisateur.
```

Même si le manifeste contient une commande PlatformIO, vérifie que l'utilisateur a explicitement autorisé le build firmware dans la session.

Si le build est autorisé :

```bash
cd esp
pio run -e pulsmon-esp32s3-display
```

Cible debug uniquement si explicitement demandée :

```bash
cd esp
pio run -e pulsmon-esp32s3-display-dev
```

Ne lance jamais `pio run -t upload`, `pio device monitor`, `idf.py flash` ou `idf.py monitor` sans instruction explicite distincte et matériel disponible.

Pour `patch_0006-1.zip`, l’instruction utilisateur et le manifeste constituent l’autorisation explicite de réaliser P8 entièrement. Après application du patch, Codex **DOIT exécuter** depuis la racine :

```bash
python3 tools/p8_validate.py --codex-full
```

Le profil impose :

```text
- précontrôle backend et suite pytest complète ;
- builds pulsmon-esp32s3-display et pulsmon-esp32s3-display-dev ;
- détection du port série, avec demande du port uniquement si plusieurs périphériques sont ambigus ;
- flash de l’environnement release ;
- monitor série borné à 180 secondes ;
- contrôles API sur http://192.168.0.10:8000 ;
- conservation de report.json, report.md et hardware-checklist.json.
```

Le premier rapport est `partial` tant que les observations physiques ne sont pas renseignées. Codex doit alors guider l’utilisateur pour réaliser les dix contrôles matériels, compléter la checklist générée, puis finaliser **le même rapport** :

```bash
python3 tools/p8_validate.py \
  --resume-report .pulsemon-results/p8/<execution>/report.json \
  --checklist-file .pulsemon-results/p8/<execution>/hardware-checklist.json \
  --require-hardware
```

Dans ce contexte P8 uniquement, Codex peut demander le port série s’il est ambigu et les observations physiques que lui seul ne peut pas voir. Il ne doit pas proposer de contourner, ignorer ou marquer automatiquement un contrôle. P8 n’est validée que si le rapport final vaut `pass`.

Sans build autorisé, limiter la validation à :

```text
- cohérence statique des includes et signatures ;
- parsing JSON et flags valid ;
- séparation réseau/cache/UI ;
- cohérence des types get_var_* ;
- absence de modification directe des sorties générées ;
- maintien de la navigation active Main/GPU/Météo ;
- absence de réintroduction implicite d’une API FAN ou d’une base backend ;
- absence de secrets ;
- tests backend d'intégration firmware présents dans api/tests si concernés.
```

---

## 16. Validation scripts et packaging

Pour chaque script shell modifié :

```bash
bash -n chemin/script.sh
```

Pour `PKGBUILD`, `pulsemon-api.service`, `pulsemon-api.conf`, `pulsemon-api.sh`, `pulsemon-api.install` ou `makepkg-tmp.sh` :

```text
- vérifier les chemins et permissions ;
- vérifier la cohérence du packaging ;
- ne pas exécuter makepkg hors /tmp ;
- ne pas lancer makepkg automatiquement sauf validation explicitement demandée ;
- ne pas modifier les dépendances du package hors patch.
```

---

## 17. Validation documentation

Si le patch modifie un comportement documenté :

```text
- vérifier la cohérence de docs/ ;
- vérifier la mise à jour correspondante de docs/fr/ ;
- vérifier README.md et README.fr.md si l'installation, les commandes ou l'architecture changent ;
- ne pas utiliser une documentation distante pour compléter le patch.
```

Une différence de traduction non liée au patch n'autorise pas une réécriture générale.

---

## 18. Gestion des tests et builds en échec

Si un test ou une validation échoue :

```text
- ne corrige rien sans nouveau patch ;
- ne refactorise pas ;
- ne modifie pas un test pour le faire passer ;
- ne réinstalle pas des dépendances au hasard ;
- ne consulte pas GitHub ;
- rapporte la commande exacte, l'erreur utile et le fichier concerné si identifiable.
```

Qualifier l'échec :

```text
lié au patch
régression probable
préexistant
environnement incomplet
dépendance absente
matériel ou capteur indisponible
commande interdite sans autorisation
test obsolète ou faux positif probable
```

---

## 19. Fichiers générés ou préexistants

Certains fichiers peuvent déjà être modifiés avant le patch.

Dans ce cas :

```text
- ne les touche pas s'ils ne sont pas ciblés ;
- ne les nettoie pas ;
- ne les signale pas comme erreur ;
- mentionne-les comme changements préexistants non ciblés.
```

Exemples :

```text
api/diagnostics/*.jsonl
esp/image/*
fichiers de sortie PlatformIO
fichiers UI générés
```

---

## 20. Patches de correction

Si un patch livré contient une erreur de packaging, un manifeste contradictoire ou un fichier manquant, le patch corrigé doit utiliser un suffixe :

```text
patch_0038-1.zip
patch_0038-2.zip
```

N'applique pas un patch de correction si le patch original a déjà été validé et si le correctif suffixé est devenu inutile. Rapporte ce fait sans modifier le dépôt.

---

## 21. Rapport final attendu

Réponds avec ce format :

```text
État initial notable :
- fichiers déjà modifiés avant patch, ou aucun

Fichiers supprimés :
- ...

Fichiers déplacés :
- ...

Fichiers remplacés/créés :
- ...

Commandes lancées :
- ...

Résultat des tests backend :
- ...

Résultat des tests UI backend :
- ...

Résultat firmware :
- non exécuté, non autorisé
- ou commande explicitement autorisée : résultat

Résultat scripts/packaging :
- ...

Erreurs ou blocages éventuels :
- aucun
- ou liste factuelle

Changements préexistants non ciblés :
- aucun
- ou liste

Archive appliquée :
- nom_du_zip
```

Le rapport doit être factuel. Ne conclus pas avec une proposition de travail supplémentaire.

En dehors de ce rapport final, ne produis aucun commentaire.
