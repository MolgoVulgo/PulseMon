# Documentation PulseMon

Ce répertoire contient la documentation française du projet.

L’anglais reste la langue par défaut dans `/docs`. Les traductions françaises sont stockées dans `/docs/fr`.

## Index

- `overview.md` — objet du projet, périmètre et modèle runtime.
- `architecture.md` — backend, firmware, transport et responsabilités.
- `api.md` — contrat API HTTP et payloads.
- `backend.md` — comportement du backend Linux.
- `firmware.md` — comportement du firmware ESP32-S3.
- `configuration.md` — configuration backend, ESP32, météo et news.
- `gpu.md` — supervision GPU AMD.
- `fans.md` — supervision et configuration ventilateurs.
- `weather-news.md` — modules météo et GNews autonomes.
- `web-configuration.md` — portail de configuration ESP32.
- `development.md` — installation, build, tests et contribution.
- `troubleshooting.md` — diagnostics, défauts courants et validations.

## Règles de documentation

- Toute documentation comportementale va dans `/docs`.
- Toute version française va dans `/docs/fr`.
- Les README racine restent courts et opérationnels.
- Les noms de champs API et chemins d’endpoints doivent rester exacts.
- Aucun secret, clé API ou valeur privée locale ne doit être documenté en clair.
