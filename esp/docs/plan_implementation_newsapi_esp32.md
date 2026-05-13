# Plan d'implementation GNews ESP32

## Objectif

Ajouter un module GNews autonome cote ESP32 pour alimenter la ligne info de
`ui_meteo`, sans dependance au backend Linux.

## P1 - Configuration

- Ajouter le stockage NVS `news`.
- Stocker la cle `gnews_key` sans jamais la retourner dans l'API web.
- Stocker les parametres V1: `provider`, `enabled`, `refresh_min`, `category`, `lang`,
  `country`, `max_items`, `slide_speed`, `max_age_days`, `last_ok_ts`, `last_error`.
- Ajouter la saisie/effacement de la cle GNews sur la page `/`.
- Exposer `gnews_key_set`, `news_max_items` et `news_slide_speed` via `GET /api/config`.

Validation:

- sauvegarde d'une cle via le portail web;
- `GET /api/config` indique seulement l'etat de presence;
- reset config supprime aussi la configuration news.

## P2 - Client HTTPS GNews

- Creer un module client dedie, separe du rendu LVGL.
- Appeler `https://gnews.io/api/v4/top-headlines`.
- Construire l'URL sans secret: `category`, `lang`, `country`, `max`, `from`.
- Utiliser le header `X-Api-Key`, jamais l'URL.
- Ajouter `User-Agent` et `Accept: application/json`.
- Respecter un timeout court et une taille max de reponse.
- Ajouter des logs sous `PULSEMON_NEWS_DEBUG` pour la requete HTTPS,
  le statut HTTP, la taille de reponse, le statut JSON et les erreurs parsees.
- Ne jamais logger la cle API ni l'URL avec secret.

Validation:

- aucun log ne contient la cle;
- absence de cle non bloquante;
- erreur TLS/HTTP mappee proprement.

## P3 - Parsing et cache

- Parser `totalArticles`, `articles[]`, `title`, `source.name`, `publishedAt`.
- Selectionner le premier titre exploitable.
- Nettoyer le texte: espaces, retours ligne, tabulations, entites simples.
- Conserver le dernier titre valide en RAM.
- Marquer le cache obsolete apres 6 heures.
- Refuser un article trop vieux, sans `publishedAt`, non UTF-8 ou hors langue
  `fr` quand le champ langue est present.

Validation:

- JSON invalide n'efface pas le dernier titre;
- reponse vide donne `empty_result`;
- titre trop court ignore.

## P4 - Cycle de service

- Ajouter une tache/timer GNews.
- Rafraichir nominalement toutes les 30 minutes.
- Ne pas appeler GNews avant Wi-Fi, DNS, SNTP valide et cle presente.
- Appliquer un backoff sur erreurs reseau, TLS, 429 et cle invalide.

Validation:

- aucun appel depuis la boucle UI;
- pas de retry en boucle;
- quota protege.

## P5 - Ligne info meteo/news

- Centraliser l'arbitrage de la ligne info.
- Priorite stricte: alerte meteo, news valide, cache news, rien.
- Faire defiler les titres de droite a gauche en scroll circulaire.
- Afficher les titres les uns apres les autres, avec `max_items` par defaut a 5.
- Rendre la vitesse de defilement configurable depuis la page web.
- Ne jamais masquer une alerte meteo active par une news.
- Garder les messages techniques hors affichage nominal.

Validation:

- alerte meteo prioritaire;
- news affichee seulement sans alerte;
- cache affiche en degrade si GNews echoue.

## Hors perimetre V1

- Images d'articles.
- Ouverture des URLs.
- Selection dynamique des sources.
- Resume IA.
- Persistance longue duree des news.
- Fallback NewsAPI.
