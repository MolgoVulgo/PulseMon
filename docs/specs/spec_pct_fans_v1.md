# Spec V1 - Calcul `pct_fans`

Source: analyse de `tmp/spec_calcul_speed_pct_fans.md`.

## Objet

Definir le calcul backend de `pct_fans` pour la vue `GET /api/v1/fans/dashboard`.

`pct_fans` represente la vitesse mecanique reelle du ventilateur dans sa plage utile configuree (`rpm_min`..`rpm_max`).

## Entrees

- `rpm`: RPM courant lu depuis la sonde.
- `rpm_min`: borne basse configuree par ventilateur.
- `rpm_max`: borne haute configuree par ventilateur.

## Formule

```text
pct_fans = clamp(round((rpm - rpm_min) * 100 / (rpm_max - rpm_min)), 0, 100)
```

## Regles de validite

- calcul autorise si `rpm` et `rpm_max` sont definis;
- si `rpm_min` est absent (`null`), la valeur effective est `0`;
- configuration invalide si `rpm_max <= rpm_min_effective`;
- en cas invalide ou RPM absent: `pct_fans = null`.

## Contraintes

- `pct_fans` est un entier dans `0..100`;
- ne pas utiliser la consigne PWM pour ce calcul;
- ne pas utiliser `min=0 RPM` systeme comme reference automatique.

## Contrat JSON

`fans[]` expose:
- `label`
- `role`
- `rpm`
- `pwm_pct`
- `pct_fans`
