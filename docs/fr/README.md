# Documentation PulseMon

Ce répertoire est la racine documentaire canonique du snapshot `PulseMon.zip` courant.

L’implémentation et les fichiers de configuration du snapshot sont la source de vérité. La documentation doit décrire le runtime existant. Les éléments historiques, générés ou planifiés doivent être isolés et qualifiés explicitement.

La documentation anglaise est la version par défaut. Le miroir français est stocké dans `docs/fr/`.

## Index

- `overview.md` — périmètre actif et non-objectifs.
- `architecture.md` — responsabilités backend/firmware et flux de données.
- `api.md` — endpoints HTTP et contrats courants.
- `backend.md` — runtime backend Linux.
- `firmware.md` — écrans actifs, polling et propriété EEZ.
- `configuration.md` — environnement backend, NVS et paramètres compilés firmware.
- `gpu.md` — télémétrie GPU AMD.
- `weather-news.md` — implémentations OpenWeather et GNews.
- `web-configuration.md` — serveur local de configuration ESP32.
- `development.md` — installation, tests, environnements firmware et génération du snapshot.
- `p8-validation.md` — workflow reproductible de build, upload, série et validation matérielle.
- `troubleshooting.md` — vérifications opérationnelles basées sur l’implémentation courante.

## Règles documentaires canoniques

- Maintenir les versions anglaise et française alignées.
- Utiliser les noms exacts des endpoints, variables d’environnement et environnements de build.
- Décrire le backend comme un service de métriques en mémoire, sans base persistante.
- Traiter RPM et pourcentage du ventilateur GPU comme de la télémétrie GPU, pas comme un sous-système FAN générique.
- Documenter l’hôte/port backend comme stockés en NVS avec fallback compilé ; la clé API backend reste ni stockée ni envoyée.
- Documenter le serveur HTTP de configuration et le DNS captif comme services limités à l’AP de configuration, pas comme services LAN permanents.
- Traiter `esp/src/ui/` comme sortie générée et `esp/eez/pulsmon/` comme données appartenant à EEZ Studio.
- Ne mentionner l’ancien écran généré inaccessible que lorsque la frontière EEZ ou le dépannage l’exige.
- Documenter les limites connues comme des faits courants jusqu’à modification du code.
