# Plan d'implementation - Fans config (API/UI locale/ESP32)

## 1. Objet

Planifier l'ajout de la supervision ventilateurs en extension du systeme actuel:
- backend Linux source de verite
- UI locale pour validation/calibration
- ESP32 comme client final d'affichage

## 2. Strategie

- Phase 1 (API) et Phase 2 (UI locale) en parallele
- Phase 3 (ESP32) apres validation fonctionnelle UI locale

## 3. Phase 1 - API

Objectifs:
- detection fiable des canaux ventilateurs
- normalisation stable
- mapping logique configurable
- exposition JSON distincte affichage vs diagnostic

Actions:
1. implementer detection `fanX_input`, labels, controleurs (`hwmon`, `amdgpu`).
2. definir modele de donnees physique + logique.
3. charger/appliquer une config externe de mapping.
4. exposer:
   - `GET /api/v1/fans/dashboard`
   - `GET /api/v1/fans/meta`
   - `GET /api/v1/fans/history` (optionnel)
5. journaliser absences/non-correspondances sans bruit excessif.

Definition of done:
- payload stable;
- valeurs `null` explicites;
- aucun mapping en dur.

## 4. Phase 2 - UI locale

Objectifs:
- valider les metriques et le mapping avant embarque.

Actions:
1. creer page ventilateurs dediee.
2. ajouter modes normal/brut/diagnostic.
3. ajouter mode calibration orientant la configuration de mapping.
4. verifier variation RPM en dynamique.

Definition of done:
- mapping valide operateur;
- coherence RPM confirmee en situation reelle.

## 5. Phase 3 - ESP32

Objectifs:
- integrer uniquement les donnees deja resolues/validees.

Actions:
1. parser `/api/v1/fans/dashboard`.
2. stocker snapshot local minimal.
3. afficher label/RPM/PWM% (si present) + stale etat.
4. conserver dernier snapshot valide en cas d'erreur.

Definition of done:
- affichage stable;
- aucune logique de matching firmware.

## 6. Validation

Backend:
- tests unitaires collecteurs/mapping/normalisation;
- tests contrat endpoints fans;
- tests non-regression endpoints existants.

UI locale:
- validation visuelle des etats (valide, non mappe, absent, stale).

Firmware:
- validation parsing + rendu + comportement hors-ligne.

## 7. Risques et mitigations

1. variabilite `hwmon` selon plateformes:
   - mitigation: matching multi-criteres + config externe
2. canaux non exploitables:
   - mitigation: separation vue technique / vue affichage
3. confusion de mapping:
   - mitigation: phase calibration obligatoire avant ESP32
