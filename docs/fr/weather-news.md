# Modules météo et actualités

Le firmware ESP32-S3 inclut des fonctions d’information autonomes qui ne dépendent pas du backend Linux une fois le Wi-Fi et les clés configurés.

## Météo

La météo est récupérée depuis OpenWeather par le firmware.

Le firmware stocke la configuration OpenWeather en NVS et met à jour les variables UI générées pour :

- heure locale ;
- date locale ;
- température courante ;
- condition météo ;
- libellés de prévision glissante ;
- icônes météo chargées depuis la carte SD.

La clé OpenWeather ne doit pas être loguée ni retournée par les endpoints de configuration.

## Icônes météo

Les icônes météo sont chargées depuis la carte SD :

- `/sdcard/icon_150.bin` pour l’icône météo principale ;
- `/sdcard/icon_50.bin` pour les icônes de prévision.

Les diagnostics de démarrage doivent valider le montage SD et le décodage des fichiers d’icônes.

## Fournisseur d’actualités

Les brèves sont récupérées directement par l’ESP32-S3 depuis GNews.

Le fournisseur est uniquement GNews. NewsAPI n’est pas utilisé en fallback.

Endpoint :

```text
https://gnews.io/api/v4/top-headlines
```

Paramètres de requête :

| Paramètre | Valeur |
|---|---|
| `category` | `general` |
| `lang` | `fr` |
| `country` | `fr` |
| `max` | `5` |
| `from` | heure UTC courante moins 15 jours, format ISO |

Header d’authentification :

```text
X-Api-Key: <clé GNews depuis NVS>
```

La clé ne doit pas être placée dans l’URL en fonctionnement normal.

## Stockage NVS news

Namespace recommandé : `news`.

Clés recommandées :

| Clé | Rôle |
|---|---|
| `provider` | fournisseur fixe, `gnews` |
| `gnews_key` | clé API GNews |
| `enabled` | activation du module |
| `refresh_min` | intervalle de rafraîchissement |
| `category` | catégorie GNews |
| `lang` | langue des titres |
| `country` | filtre pays |
| `max_items` | nombre maximal d’articles demandés |
| `slide_speed` | vitesse du ticker |
| `max_age_days` | âge maximal des articles |
| `last_ok_ts` | timestamp du dernier succès |
| `last_error` | code compact de dernière erreur |

Comportement par défaut :

- rafraîchissement toutes les 30 minutes ;
- demande de 5 articles maximum ;
- rejet des articles de plus de 15 jours ;
- conservation du dernier titre valide en cache ;
- ligne vide si aucun contenu valide n’existe.

## Préconditions d’appel news

Avant d’appeler GNews, le firmware doit avoir :

- Wi-Fi connecté ;
- DNS fonctionnel ;
- heure SNTP valide ;
- clé GNews stockée en NVS ;
- module news activé ;
- intervalle de rafraîchissement écoulé ;
- aucun backoff actif.

L’heure SNTP est obligatoire pour la validation TLS et le calcul du paramètre `from`.

## Validation article

Un article valide doit avoir :

- un titre non vide de plus de 10 caractères ;
- une valeur `publishedAt` parsable ;
- une date de publication dans la fenêtre maximale ;
- `lang=fr` si le champ est présent ;
- un texte UTF-8 valide.

L’UI utilise uniquement les titres nettoyés. Elle n’affiche pas description, contenu, URL ou image.

## Priorité ligne d’information

La ligne d’information applique cette priorité :

1. alerte météo ;
2. brève valide ;
3. brève en cache ;
4. ligne vide.

Une alerte météo masque toujours les news. Les erreurs techniques restent dans les diagnostics et les logs, pas dans la ligne nominale.

## Backoff

Comportement de retry recommandé :

- erreur réseau, TLS ou serveur temporaire : retry après intervalle long normal ;
- limite de requêtes : backoff long ;
- quota ou autorisation : pas de retry agressif ;
- parsing ou résultat vide : conserver le cache et réessayer plus tard.
