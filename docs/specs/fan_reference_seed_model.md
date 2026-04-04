# Modele JSON - fan_reference_seed

Document de reference pour `tmp/fan_reference_seed.json`.

## Objet

Ce fichier est un catalogue materiel de modeles de ventilateurs:
- source de donnees de reference (constructeur / fiche utilisateur);
- aide a la configuration UI;
- pas un mapping runtime direct des headers `hwmon` Linux.

## Structure racine

- `generated_at`: date de generation.
- `scope`: contexte de collecte.
- `schema`: description des champs d'un item ventilateur.
- `data`: dictionnaire hierarchique `brand -> series -> [items]`.

## Structure d'un item ventilateur

- `model`: nom commercial du modele (nullable).
- `size_mm`: taille (nullable).
- `thickness_mm`: epaisseur (nullable).
- `pwm`: support PWM (nullable).
- `rpm_min`, `rpm_max`: plage nominale (nullable).
- `connector`: type de connecteur (nullable).
- `airflow_cfm`, `airflow_m3h`: debit d'air (nullable).
- `static_pressure_mm_h2o`: pression statique (nullable).
- `noise_db_a`: bruit nominal (nullable).
- `voltage_v`: plage tension (nullable, texte).
- `current_a`: courant nominal (nullable).
- `power_w`: puissance nominale (nullable).
- `source_type`: `official_page` ou `user_datasheet`.
- `source_note`: note de provenance.

## Regles de qualite

- Valeurs inconnues: `null` explicite.
- Ne pas inventer de valeur.
- Conserver la provenance (`source_type`, `source_note`).
- Stabilite de schema: meme clef meme sens, meme unite.

## Difference avec la config runtime fans

`fan_reference_seed.json`:
- catalogue produit (marque/serie/modele).
- independant de la machine Linux cible.

`fans_mapping.json` runtime:
- mapping machine locale (`hwmon_name`, `channel`, `group`).
- active/desactive et ordonne les ventilateurs affiches.
- persiste dans `~/.config/pulsemon/fans_mapping.json` par defaut.
