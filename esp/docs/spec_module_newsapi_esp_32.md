# Spécification d’implémentation — Module News autonome ESP32

## 1. Objet

Mettre à jour le module d’actualité autonome ESP32 en remplaçant NewsAPI comme fournisseur principal par GNews API.

Objectif fonctionnel : afficher une brève utile sur la ligne météo/info lorsque aucune alerte météo n’est active.

Le module doit rester autonome :

- pas de dépendance au PC Linux ;
- appel HTTPS direct depuis l’ESP32 ;
- clé API stockée en NVS ;
- clé jamais affichée ;
- clé jamais loguée ;
- cache local conservé en cas d’échec réseau ou API.

Le module météo est considéré comme finalisé. Cette spécification ne le modifie pas.

---

## 2. Décision d’architecture

Architecture retenue :

- ESP32 connecté au Wi-Fi ;
- synchronisation horaire via SNTP ;
- appel HTTPS direct vers GNews API ;
- clé GNews stockée en NVS ;
- parsing JSON local ;
- cache RAM de la dernière brève valide ;
- affichage uniquement si aucune alerte météo n’est active.

Décision fournisseur :

| Rôle | Fournisseur |
|---|---|
| Principal | GNews API |
| Fallback provider | aucun |
| V1 recommandée | GNews seul |

NewsAPI est exclu du module. Il ne doit pas être implémenté comme fallback.

---

## 3. Fournisseur principal : GNews API

Endpoint principal :

`GET https://gnews.io/api/v4/top-headlines`

Motifs :

- endpoint prévu pour les titres d’actualité courants ;
- classement basé sur le ranking Google News ;
- filtrage par langue ;
- filtrage par pays ;
- limitation du nombre d’articles ;
- filtrage temporel via `from` ;
- réponse JSON simple.

URL cible sans clé dans l’URL :

`https://gnews.io/api/v4/top-headlines?category=general&lang=fr&country=fr&max=5&from=<UTC_NOW_MINUS_15D_ISO>`

Header HTTP obligatoire :

`X-Api-Key: <clé GNews>`

La clé ne doit pas être passée dans l’URL en fonctionnement normal.

---

## 4. URL V1 recommandée

Au 13 mai 2026, avec une fenêtre maximale de 15 jours :

`https://gnews.io/api/v4/top-headlines?category=general&lang=fr&country=fr&max=5&from=2026-04-28T00:00:00Z`

En fonctionnement réel, la date ne doit pas être figée.

Calcul dynamique :

`from = now_utc - 15 jours`

Format requis :

`YYYY-MM-DDTHH:MM:SSZ`

Exemple dynamique attendu :

`2026-04-28T00:00:00Z`

---

## 5. Paramètres GNews retenus

| Paramètre | Valeur V1 | Rôle |
|---|---:|---|
| `category` | `general` | actualité généraliste |
| `lang` | `fr` | articles en français |
| `country` | `fr` | sources françaises ou articles pertinents pour la France |
| `max` | `5` | maximum 5 articles |
| `from` | `now_utc - 15j` | exclut les articles trop anciens |

Paramètres non retenus en V1 :

| Paramètre | Motif |
|---|---|
| `q` | pas nécessaire pour des top headlines généralistes |
| `to` | inutile sauf debug ou fenêtre fermée |
| `page` | inutile pour 5 articles |
| `nullable` | inutile si seuls `title`, `source.name`, `publishedAt` sont utilisés |
| `truncate` | inutile car `content` ignoré |

---

## 6. Décision fournisseur

GNews est l’unique fournisseur d’actualité en V1.

NewsAPI est exclu :

- pas de fallback NewsAPI ;
- pas de double provider ;
- pas de logique de bascule API ;
- pas de clé NewsAPI en NVS ;
- pas d’appel NewsAPI dans le firmware.

Motifs : simplification du firmware, réduction des cas d’erreur, contrat plus net côté UI.

---|---:|---:|
| Titres courts | oui | oui |
| Langue française | oui via `/everything` | oui via `lang=fr` |
| Pays France | oui selon endpoint | oui via `country=fr` |
| Ranking utile | limité / ambigu | ranking Google News |
| Récence | possible mais moins nette | `top-headlines` + `from` |
| Format JSON | simple | simple |
| Clé en header | oui | oui |
| Usage ESP32 | acceptable | préféré |

Décision : GNews est le fournisseur principal pour la V1.

---

## 7. Authentification

Méthode retenue : header HTTP `X-Api-Key`.

Header :

`X-Api-Key: <clé GNews>`

Méthode autorisée mais non retenue :

`apikey=<clé>` dans la query string.

Motif du rejet en fonctionnement normal : la clé dans l’URL peut fuiter dans les logs, traces réseau, dumps ou messages de debug.

Règles :

- ne jamais afficher la clé ;
- ne jamais logger la clé ;
- ne jamais stocker une URL complète contenant la clé ;
- ne jamais exposer la clé via écran diagnostic ;
- prévoir une suppression/reset de clé.

---

## 8. Stockage NVS

Namespace recommandé :

`news`

Clés NVS recommandées :

| Clé | Type | Rôle |
|---|---:|---|
| `provider` | string | fournisseur actif : `gnews` |
| `gnews_key` | string | clé GNews |
| `enabled` | bool | activation du module |
| `refresh_min` | int | cadence de rafraîchissement |
| `category` | string | catégorie GNews |
| `lang` | string | langue |
| `country` | string | pays |
| `max_items` | int | nombre maximal d’articles |
| `max_age_days` | int | âge maximal des articles |
| `last_ok_ts` | int | timestamp dernier succès |
| `last_error` | string | dernier état d’erreur |

Valeurs par défaut :

| Paramètre | Valeur |
|---|---:|
| `provider` | `gnews` |
| `enabled` | true |
| `refresh_min` | 30 |
| `category` | `general` |
| `lang` | `fr` |
| `country` | `fr` |
| `max_items` | 5 |
| `max_age_days` | 15 |

La clé `gnews_key` n’a pas de valeur par défaut.

Si `gnews_key` est absente, le module news reste inactif mais le firmware continue de fonctionner normalement.

---

## 9. Niveau de sécurité retenu

Contexte : développement personnel, appareil local, risque acceptable.

Sécurité V1 obligatoire :

- clé stockée en NVS ;
- clé jamais affichée ;
- clé jamais loguée ;
- clé envoyée en header ;
- logs masqués ;
- aucun dump de configuration complet contenant la clé.

Sécurité optionnelle ultérieure :

- NVS encryption ;
- flash encryption ;
- secure boot.

Ces options ne sont pas bloquantes en V1.

---

## 10. Cadence de rafraîchissement

Cadence recommandée : 30 minutes.

Objectif : limiter les appels API et préserver le quota.

| Cadence | Appels/jour | Statut |
|---:|---:|---|
| 15 min | 96 | limite haute |
| 30 min | 48 | recommandé |
| 60 min | 24 | conservateur |

Règles :

- aucun appel déclenché par le rendu UI ;
- aucun appel à chaque refresh écran ;
- aucun retry agressif ;
- backoff long en cas de quota dépassé ;
- conservation du cache si erreur.

---

## 11. Préconditions réseau

Avant tout appel GNews :

1. Wi-Fi connecté ;
2. DNS fonctionnel ;
3. heure système synchronisée via SNTP ;
4. clé GNews présente en NVS ;
5. module activé ;
6. délai de rafraîchissement écoulé ;
7. aucun backoff bloquant actif.

La synchronisation SNTP est obligatoire pour éviter les erreurs de validation TLS.

Si l’heure n’est pas valide, ne pas appeler GNews.

---

## 12. Requête HTTPS

Méthode :

`GET`

URL :

`https://gnews.io/api/v4/top-headlines?category=general&lang=fr&country=fr&max=5&from=<from_iso>`

Headers :

| Header | Valeur |
|---|---|
| `X-Api-Key` | clé lue depuis NVS |
| `User-Agent` | nom court du firmware |
| `Accept` | `application/json` |

Timeouts recommandés :

| Élément | Valeur |
|---|---:|
| timeout connexion | 5 s |
| timeout lecture | 8 s |
| taille max réponse | 16 à 32 Ko |

La réponse doit être rejetée si elle dépasse la taille maximale autorisée.

---

## 13. Réponse JSON GNews exploitée

Structure utile :

```json
{
  "totalArticles": 123,
  "articles": [
    {
      "title": "Titre de l’article",
      "description": "Description courte",
      "content": "Contenu tronqué ou complet selon plan",
      "url": "https://...",
      "image": "https://...",
      "publishedAt": "2026-05-13T12:00:00Z",
      "lang": "fr",
      "source": {
        "name": "Nom source",
        "url": "https://..."
      }
    }
  ]
}
```

Champs utilisés en V1 :

| Champ | Usage |
|---|---|
| `totalArticles` | diagnostic uniquement |
| `articles[]` | liste des articles |
| `articles[].title` | texte principal affichable |
| `articles[].publishedAt` | contrôle de fraîcheur |
| `articles[].lang` | validation langue si présent |
| `articles[].source.name` | source courte |

Champs ignorés en V1 :

- `description` ;
- `content` ;
- `url` ;
- `image` ;
- `source.url`.

L’ESP32 ne télécharge aucune image.

---

## 14. Validation des articles

Un article est valide si :

- `title` existe ;
- `title` n’est pas vide ;
- `title` contient plus de 10 caractères ;
- `publishedAt` existe et est parsable ;
- `publishedAt >= now_utc - 15 jours` ;
- `lang` vaut `fr` si le champ est présent ;
- le texte est UTF-8 valide ;
- le titre n’est pas manifestement bruité.

Règle de sélection V1 :

1. lire les articles dans l’ordre reçu ;
2. ignorer les articles invalides ;
3. sélectionner le premier article valide ;
4. conserver optionnellement jusqu’à 5 titres valides pour rotation future ;
5. mettre à jour le cache avec le titre sélectionné.

---

## 15. Nettoyage du titre

Règles de nettoyage :

- supprimer les retours ligne ;
- remplacer les tabulations par des espaces ;
- compresser les espaces multiples ;
- supprimer les espaces en début/fin ;
- décoder les entités HTML simples si nécessaire ;
- ne pas afficher d’URL ;
- ne pas afficher `description` ou `content` en V1 ;
- tronquer proprement selon la largeur UI.

Longueur cible :

| Mode UI | Longueur |
|---|---:|
| ligne fixe | 80 caractères |
| ticker défilant | 120 caractères |
| écran large | 160 caractères max |

Valeur V1 recommandée : 120 caractères maximum.

---

## 16. Modèle interne du module news

Structure logique recommandée :

| Champ | Type | Rôle |
|---|---:|---|
| `provider` | enum | `gnews` |
| `kind` | enum | `news` |
| `text` | string/null | titre nettoyé |
| `source` | string/null | source courte |
| `published_ts` | int/null | timestamp publication UTC |
| `fetch_ts` | int | timestamp récupération UTC |
| `valid` | bool | validité fonctionnelle |
| `stale` | bool | donnée ancienne |
| `error` | enum/null | dernier état d’erreur |

Valeurs possibles de `error` :

| Valeur | Sens |
|---|---|
| `none` | aucun problème |
| `disabled` | module désactivé |
| `missing_key` | clé absente |
| `wifi_down` | Wi-Fi indisponible |
| `time_invalid` | SNTP non synchronisé |
| `tls_error` | erreur TLS |
| `http_400` | requête invalide |
| `http_401` | clé absente ou invalide |
| `http_403` | quota journalier dépassé ou abonnement bloqué |
| `http_429` | trop de requêtes sur une courte période |
| `http_5xx` | erreur serveur distante |
| `parse_error` | JSON inexploitable |
| `empty_result` | aucun article valide |
| `invalid_article` | articles présents mais non exploitables |

---

## 17. Cache local

Le module doit conserver la dernière brève valide.

Cache RAM minimal :

| Champ | Rôle |
|---|---|
| dernier titre valide | fallback affichable |
| source | contexte court |
| published_ts | fraîcheur article |
| fetch_ts | fraîcheur récupération |
| valid | état logique |
| stale | obsolescence |

Cache persistant optionnel :

- dernier titre valide ;
- source ;
- timestamp publication ;
- timestamp récupération.

Le cache persistant n’est pas obligatoire en V1.

Comportement attendu :

- si GNews échoue, conserver la dernière brève valide ;
- si aucun titre n’a jamais été récupéré, afficher ligne vide ;
- si le cache est ancien, marquer `stale=true` ;
- ne jamais vider brutalement la ligne sur erreur API.

Seuil recommandé pour `stale` : 6 heures depuis `fetch_ts`.

---

## 18. Intégration avec la ligne météo / info

La ligne d’affichage reste générique.

Priorité :

| Priorité | Type | Source |
|---:|---|---|
| 1 | `weather_alert` | module météo |
| 2 | `news` | GNews valide |
| 3 | `cached_news` | cache GNews |
| 4 | `none` | aucun contenu affiché |

Règle ferme : une alerte météo active masque toujours la news.

Si aucune alerte météo n’est active et qu’aucune news valide n’est disponible, la ligne reste vide. Aucun message de remplacement ne doit être affiché.

Pseudo-contrat UI :

| Champ | Rôle |
|---|---|
| `line_kind` | type de contenu affiché |
| `line_text` | texte final affichable |
| `line_source` | source courte |
| `line_stale` | donnée ancienne ou non |

La logique de priorité doit être centralisée dans un composant d’orchestration UI, pas dispersée dans les widgets LVGL.

Rendu LVGL attendu :

- le texte final est publié dans la variable générée `ui_meteo_alert` ;
- le label associé doit rester en `LV_LABEL_LONG_SCROLL_CIRCULAR` ;
- la vitesse de défilement est issue de la configuration `slide_speed` ;
- la réapplication du texte peut être évitée si le texte et la vitesse sont inchangés, mais la configuration LVGL doit rester garantie après toute réinitialisation ou recréation d’objet.

La ligne peut concaténer plusieurs titres valides. Dans ce cas, le séparateur affiché doit rester sobre et stable pour limiter la largeur et les allocations.

---

## 19. États d’affichage

| État | Affichage recommandé |
|---|---|
| alerte météo active | texte alerte météo |
| news valide | titre GNews |
| news obsolète | dernier titre connu avec indicateur discret |
| clé absente | ligne vide |
| quota dépassé | ligne vide ou cache valide si disponible |
| erreur réseau | ligne vide ou cache valide si disponible |
| aucun cache | ligne vide |

En affichage normal, éviter les messages techniques.

Les erreurs détaillées doivent rester dans un écran diagnostic ou dans des logs masqués. Elles ne doivent jamais produire un message visible sur la ligne météo/info.

---

## 20. Gestion des erreurs GNews

Codes HTTP à gérer :

| Code | Sens | Comportement |
|---:|---|---|
| 200 | succès | parser JSON |
| 400 | requête invalide | marquer bug config, backoff long |
| 401 | clé absente/invalide | configuration invalide, pas de retry agressif |
| 403 | quota journalier atteint ou accès interdit | backoff jusqu’au prochain reset probable |
| 429 | trop de requêtes | backoff long |
| 500 | erreur serveur | retry au prochain cycle normal |
| 503 | maintenance/indisponible | retry au prochain cycle normal |

Format d’erreur attendu :

```json
{
  "errors": [
    "message d’erreur"
  ]
}
```

Ou :

```json
{
  "errors": {
    "attribute": "message d’erreur"
  }
}
```

Le contenu exact de `errors` est diagnostic. L’UI normale ne doit pas l’afficher.

Backoff recommandé :

| Erreur | Délai minimal avant retry |
|---|---:|
| réseau temporaire | 30 min |
| TLS | 30 min |
| 500 / 503 | 30 min |
| 429 | 6 h |
| 403 quota | jusqu’au prochain jour UTC ou 6 h minimum |
| 401 | pas de retry automatique fréquent |
| 400 | pas de retry automatique fréquent |

---

## 21. Quota et limitation des appels

Règles :

- aucun appel tant que l’heure n’est pas synchronisée ;
- aucun appel si le dernier succès est trop récent ;
- aucun appel répété en boucle en cas d’erreur ;
- aucun appel déclenché par le rendu UI ;
- cadence nominale : 1 appel toutes les 30 minutes ;
- objectif nominal : 48 appels/jour maximum.

Compteur local optionnel :

| Champ | Rôle |
|---|---|
| `requests_today` | nombre d’appels depuis minuit UTC |
| `day_marker` | jour UTC courant |
| `last_http_status` | dernier code HTTP |

Ce compteur est optionnel mais recommandé pour diagnostic.

---

## 22. Organisation firmware recommandée

Découpage logique :

| Bloc | Responsabilité |
|---|---|
| `news_config` | lecture/écriture NVS |
| `news_provider_gnews` | construction URL et appel HTTPS GNews |
| `news_parser_gnews` | parsing JSON GNews |
| `news_store` | cache mémoire |
| `news_service` | cycle refresh, backoff, erreurs |
| `info_line` | arbitrage météo/news/cache |
| `ui_status` | affichage diagnostic si nécessaire |

Le rendu LVGL ne doit jamais appeler directement le client HTTPS.

Les fichiers sous `src/ui/` sont générés par EEZ. Toute évolution de la ligne info qui nécessite un nouveau glyphe, une police ou une variable UI doit être portée dans `eez/pulsmon/pulsmon.eez-project`, puis propagée par génération. Les fichiers générés ne doivent pas être édités comme source fonctionnelle.

---

## 23. Cycle nominal

Séquence :

1. démarrage ESP32 ;
2. initialisation NVS ;
3. lecture configuration news ;
4. connexion Wi-Fi ;
5. synchronisation SNTP ;
6. calcul `from = now_utc - 15 jours` ;
7. attente de l’échéance de refresh ;
8. appel HTTPS GNews ;
9. contrôle code HTTP ;
10. parsing JSON ;
11. validation des articles ;
12. sélection du premier article valide ;
13. nettoyage du titre ;
14. mise à jour du cache ;
15. notification UI ;
16. attente du prochain cycle.

---

## 24. Critères d’acceptation

Le module est conforme si :

1. l’ESP32 appelle directement GNews sans dépendre du PC Linux ;
2. la clé GNews est lue depuis NVS ;
3. la clé n’apparaît jamais dans l’UI ;
4. la clé n’apparaît jamais dans les logs ;
5. l’authentification utilise `X-Api-Key` ;
6. l’URL utilise `top-headlines` ;
7. les articles demandés sont en français via `lang=fr` ;
8. les articles ciblent la France via `country=fr` ;
9. le nombre d’articles est limité à 5 ;
10. les articles plus anciens que 15 jours sont exclus via `from` ;
11. une alerte météo reste prioritaire ;
12. une erreur GNews ne vide pas brutalement la ligne ;
13. le dernier titre valide est conservé ;
14. les appels sont limités à une cadence maîtrisée ;
15. le rendu UI reste découplé du réseau ;
16. aucun appel HTTPS n’est lancé depuis la boucle d’affichage ;
17. l’absence de clé ne bloque pas le reste du firmware.

---

## 25. Paramètres V1 figés

| Élément | Valeur V1 |
|---|---|
| Provider | GNews |
| Endpoint | `/api/v4/top-headlines` |
| Base URL | `https://gnews.io` |
| Category | `general` |
| Langue | `fr` |
| Pays | `fr` |
| Max articles | `5` |
| Âge max | `15 jours` |
| Auth | `X-Api-Key` |
| Stockage clé | NVS |
| Refresh | 30 min |
| Texte affiché | premier `articles[].title` valide |
| Source affichée | `articles[].source.name` |
| Cache | dernier titre valide si encore acceptable |
| Aucun contenu | ligne vide |
| Priorité météo | absolue |

---

## 26. Hors périmètre V1

Ne pas implémenter en V1 :

- téléchargement d’images ;
- affichage de `description` ou `content` ;
- ouverture des URLs ;
- navigateur embarqué ;
- scrolling multi-articles complexe ;
- choix dynamique des sources depuis l’UI ;
- résumé IA ;
- stockage long terme des news ;
- chiffrement avancé obligatoire ;
- dépendance au backend Linux ;
- fallback NewsAPI ;
- message de remplacement visible du type `News indisponible` ;
- affichage d’erreurs techniques sur la ligne météo/info.

---

## 27. Décision finale

Le module news V1 utilise GNews API comme fournisseur principal.

Il appelle directement :

`https://gnews.io/api/v4/top-headlines?category=general&lang=fr&country=fr&max=5&from=<now_utc_minus_15_days>`

La clé est stockée en NVS et envoyée via header :

`X-Api-Key: <clé GNews>`

Le module récupère jusqu’à 5 titres français récents, sélectionne le premier article valide, nettoie le titre, puis le met à disposition de la ligne météo/info.

La règle d’affichage reste inchangée : l’alerte météo prime toujours sur la brève d’actualité.

Si aucune alerte météo n’est active et qu’aucune news valide n’est disponible, la ligne reste vide. NewsAPI n’est pas utilisé en fallback.
