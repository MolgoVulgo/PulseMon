# Documentation PulseMon

Point d'entree principal du projet:
- `README.md` (racine du depot)

Cette arborescence est la source documentaire active du depot.

## Structure

- `docs/specs/` : specifications fonctionnelles et contrats d'integration.
- `docs/plans/` : plans d'implementation.
- `docs/adr/` : decisions d'architecture.
- `docs/reports/` : rapports techniques et audits dates.
- `docs/archive/` : documents retires du flux actif.

## References normatives

- `docs/specs/cahier_des_charges_stats_linux_esp_32.md`
- `docs/specs/cahier_fonctionnel_stats_linux_esp_32.md`
- `docs/plans/plan_implementation_api_stats_linux.md`
- `docs/specs/spec_esp32_integration_stats_linux.md`
- `docs/specs/cahier_gpu_amd_v1.md`
- `api/docs/API_CONTRACT_V1.md` (contrat HTTP effectivement implemente)
- `api/docs/API_GPU_CONTRACT_V1.md` (extension GPU dediee)

## Regle pratique

Si un document de `docs/specs/` diverge du code API, la verite d'interface est `api/docs/API_CONTRACT_V1.md` et les tests de contrat backend.
