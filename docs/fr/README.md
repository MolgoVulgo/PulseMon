# Documentation PulseMon

Ce répertoire est la racine documentaire canonique du snapshot `PulseMon.zip` courant.

L’implémentation et les fichiers de configuration du snapshot sont la source de vérité. La documentation décrit le runtime réellement présent ; tout comportement prévu, dormant ou abandonné doit être identifié explicitement.

La documentation anglaise est la référence par défaut. Le miroir français se trouve dans `docs/fr/`.

## Index

- `overview.md` — périmètre actif, composants conservés et hors périmètre.
- `architecture.md` — responsabilités backend/firmware et flux de données.
- `api.md` — endpoints HTTP et contrats de payload courants.
- `backend.md` — runtime backend Linux.
- `firmware.md` — écrans actifs, polling et propriété EEZ.
- `configuration.md` — environnement backend, SQLite, NVS et paramètres compilés firmware.
- `gpu.md` — télémétrie GPU AMD.
- `fans.md` — sous-système FAN conservé mais inactif.
- `weather-news.md` — implémentations OpenWeather et GNews courantes.
- `web-configuration.md` — serveur local de configuration ESP32.
- `development.md` — installation, tests, environnements firmware et génération du snapshot.
- `troubleshooting.md` — vérifications opérationnelles basées sur l’existant.

## Règles documentaires

- Aligner les versions anglaise et française.
- Utiliser les noms exacts des endpoints, variables d’environnement et environnements de build.
- Ne pas présenter FAN comme une fonctionnalité firmware active.
- Ne pas indiquer que l’adresse backend ou la clé API backend est stockée en NVS.
- Traiter `esp/src/ui/` comme une sortie générée et `esp/eez/pulsmon/` comme des données pilotées par EEZ Studio.
- Documenter les défauts connus comme des faits tant que le code ne les a pas corrigés.
