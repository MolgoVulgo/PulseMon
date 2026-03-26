# Documentation PulseMon

Cette arborescence est la source documentaire active du dépôt.

## Structure

- `docs/specs/` : spécifications fonctionnelles et contrats d'intégration.
- `docs/plans/` : plans d'implémentation.
- `docs/adr/` : décisions d'architecture.
- `docs/reports/` : rapports techniques et audits datés.
- `docs/archive/` : documents retirés du flux actif.

## Cohérence vérifiée

Audit réalisé le 2026-03-26 sur l'ancien dossier `Docs/`.

Incohérences corrigées:
- endpoints ESP32 documentés en `/api/stats` alors que le backend expose `/api/v1/dashboard` et `/api/v1/history`;
- exemples JSON `dashboard/history` non alignés avec `api/docs/API_CONTRACT_V1.md`;
- conflit de gouvernance documentaire (`spec` déclarée source unique en opposition avec les cahiers et le plan actifs).

Références normatives actuelles:
- `docs/specs/cahier_des_charges_stats_linux_esp_32.md`
- `docs/specs/cahier_fonctionnel_stats_linux_esp_32.md`
- `docs/plans/plan_implementation_api_stats_linux.md`
- `api/docs/API_CONTRACT_V1.md` (contrat HTTP effectivement implémenté)

## Règle pratique

Si un document de `docs/specs/` diverge du code API, la vérité d'interface est `api/docs/API_CONTRACT_V1.md` et les tests de contrat backend.
