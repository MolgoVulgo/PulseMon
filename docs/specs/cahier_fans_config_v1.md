# Cahier des charges et fonctionnel - Supervision ventilateurs V1

## 1. Objet

Ajouter la supervision de tous les ventilateurs exploitables de la configuration cible, sans rupture de l'architecture existante:
- API Linux
- UI locale de validation
- integration ESP32-S3

Le chantier est decoupe en 3 phases:
1. API (detection, normalisation, exposition)
2. UI locale (validation/correction)
3. ESP32 (consommation de la vue validee)

Regle de sequence:
- phases 1 et 2 peuvent avancer en parallele;
- phase 3 commence apres validation UI locale.

## 2. Principes directeurs

1. aucune valeur inventee;
2. aucune hypothese fixe sur le nombre de ventilateurs;
3. separation stricte detection physique / mapping logique / vue finale;
4. mapping logique configurable (pas code en dur);
5. ESP32 limite a l'affichage, sans logique de calibration.

## 3. Perimetre V1

Ventilateurs cibles (si exposes Linux):
- CPU fan
- fans carte mere/chassis
- pump (si presente)
- fans GPU AMD
- autres headers exposes via hwmon/sysfs/drm

Hors garantie:
- hubs sans tachymetre
- ventilateurs sans retour RPM
- dispositifs proprietaires non exposes Linux

## 4. Exigences API

### 4.1 Detection et lecture

- scanner `hwmon` pertinent + `amdgpu`;
- detecter `fanX_input`;
- lire `fanX_label` si present;
- conserver la source exacte et le controleur.

### 4.2 Modeles de vue

L'API doit separer:
- vue technique (diagnostic/calibration);
- vue d'affichage (clients finaux/ESP32).

### 4.3 Endpoint d'affichage

`GET /api/v1/fans/dashboard`

Regles:
- liste de ventilateurs mappes, valides, ordonnes;
- ventilateur non mappe/non valide/non retenu: absent de cette vue.

Structure cible:

```json
{
  "v": 1,
  "ts": 1774256402,
  "fans": [
    { "id": "cpu_fan", "label": "CPU", "rpm": 1132, "pwm_pct": 45 }
  ]
}
```

### 4.4 Endpoint technique

`GET /api/v1/fans/meta`

Doit permettre:
- inspection canaux physiques;
- source, validite, connected;
- mapping et statut `configured`;
- details controleur et matching.

### 4.5 Endpoint history (optionnel V1)

`GET /api/v1/fans/history`

Optionnel en premiere iteration.

### 4.6 Regles de validite

Vue affichage:
- uniquement les ventilateurs exploitables pour UI finale.

Vue technique:
- canaux connus conserves;
- valeur absente -> `null`;
- `valid=false` explicite si donnee non fiable.

## 5. Mapping configurable

Le mapping doit etre charge depuis une configuration externe.

Champs minimaux:
- `id`
- `label`
- `role`
- `order`
- `enabled`/`hidden`

Matching recommande:
- `hwmon_name`
- chemin `hwmon`
- infos DMI / vendor / model
- type de controleur

## 6. Exigences UI locale

Objectif:
- valider visuellement et empiriquement les metriques ventilateurs.

Capacites minimales:
- vue normale (ordonnee par mapping);
- vue brute (canaux physiques);
- vue diagnostic (source/validite/mapping);
- mode calibration (aide au mapping).

## 7. Exigences ESP32

L'ESP32 consomme uniquement la vue d'affichage validee:
- `label`
- `rpm`
- `pwm_pct` si disponible
- etat global de fraicheur

Regles:
- pas de remapping cote firmware;
- conserver le dernier snapshot valide en cas d'echec;
- signaler l'obsolescence.

## 8. Criteres d'acceptation

Conforme si:
1. l'API detecte les ventilateurs reels exposes;
2. chaque canal remonte avec source identifiable;
3. RPM plausibles et verifiables;
4. la vue locale permet validation mapping;
5. la configuration est modifiable sans changer le code metier;
6. l'ESP32 ne reinterprete pas la logique Linux;
7. pas de regression sur l'API existante.

## 9. Hors perimetre V1

- controle courbes ventilateurs
- ecriture PWM
- BIOS/UEFI tuning
- overclocking/undervolt
- support exhaustif de tous hubs proprietaires
