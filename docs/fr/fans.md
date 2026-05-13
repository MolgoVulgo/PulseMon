# Supervision et configuration ventilateurs

PulseMon supporte la télémétrie ventilateurs et un mapping configurable. Le backend expose les valeurs runtime et des endpoints de configuration locale pour l’UI admin.

## Comportement runtime

La télémétrie ventilateurs peut inclure des valeurs RPM et des pourcentages calculés. La valeur `pct_fans` est basée sur `rpm_min` et `rpm_max` quand la configuration est valide.

Si la configuration manque ou est invalide, les pourcentages calculés doivent rester nullables au lieu d’être fabriqués.

## Mapping

Le mapping associe les canaux matériels détectés à des ventilateurs logiques.

Une entrée de mapping doit permettre d’identifier le canal, définir les métadonnées d’affichage et calculer un pourcentage à partir de bornes RPM.

Si la base de configuration est vide, le backend peut générer un mapping initial depuis les canaux détectés ou importer un mapping legacy JSON si configuré.

## Endpoints API

- `GET /api/v1/fans/dashboard` — données ventilateurs courantes ;
- `GET /api/v1/fans/meta` — capacités et métadonnées détectées ;
- `GET /api/v1/fans/config` — mapping courant ;
- `PUT /api/v1/fans/config` — mise à jour du mapping ;
- `GET /api/v1/fans/reference` — références utilisées par l’UI de sélection.

Les endpoints d’administration base locale peuvent exister pour l’UI backend. Ils ne doivent pas être considérés comme des dépendances d’affichage firmware.

## Comportement UI

L’UI backend peut fournir configuration et sélection de références ventilateurs. Le polling ne doit pas réinitialiser les sélections en cours dans les formulaires.

Des pages fan peuvent rester présentes dans les sources UI générées côté firmware. La navigation et le polling runtime doivent exposer seulement les écrans réellement connectés à des données valides.

## Payload dashboard ventilateurs

`GET /api/v1/fans/dashboard` retourne uniquement les ventilateurs mappés, valides et actifs.

Chaque item ventilateur peut contenir :

- `label` ;
- `role` ;
- `rpm` ;
- `pwm_pct` ;
- `pct_fans`.

`pct_fans` est calculé si `rpm` et `rpm_max` sont disponibles. Si `rpm_min` est absent, le backend utilise `0` comme minimum effectif.

Formule :

```text
pct_fans = clamp(round((rpm - rpm_min_effective) * 100 / (rpm_max - rpm_min_effective)), 0, 100)
```

Un canal détecté avec `rpm = 0` est considéré off et n’apparaît pas dans le dashboard. Au bootstrap, un canal off est stocké avec `enabled=false`.

## Payload métadonnées ventilateurs

`GET /api/v1/fans/meta` retourne les canaux techniques avec :

- `channel` ;
- `hwmon_name` ;
- `hwmon_path` ;
- `source` ;
- `group` ;
- `label` ;
- `rpm` ;
- `pwm_pct` ;
- `connected` ;
- `valid` ;
- `error` ;
- champs `mapping` comme `configured`, `label`, `role`, `order` et `enabled`.

## Modèle de référence ventilateur

Le catalogue de référence ventilateurs contient des items aplatis avec :

- `id`, généralement construit depuis marque, série et modèle ;
- `brand` ;
- `series` ;
- `model` ;
- `rpm_min` ;
- `rpm_max` ;
- `pwm` ;
- `connector` ;
- `size_mm`.
