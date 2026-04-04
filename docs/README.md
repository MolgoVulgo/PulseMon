# Documentation PulseMon

Point d'entree principal:
- `README.md` (racine)

## Structure

- `docs/specs/`: specifications fonctionnelles et integration backend/firmware.
- `docs/plans/`: plan d'implementation et etat d'avancement.
- `docs/adr/`: decisions d'architecture.
- `docs/reports/`: rapports dates (photographies historiques).

## References normatives actives

- `api/docs/API_CONTRACT_V1.md`
- `api/docs/API_GPU_CONTRACT_V1.md`
- `docs/specs/spec_esp32_integration_stats_linux.md`
- `docs/specs/cahier_des_charges_stats_linux_esp_32.md`
- `docs/specs/cahier_fonctionnel_stats_linux_esp_32.md`
- `docs/specs/cahier_gpu_amd_v1.md`
- `docs/specs/cahier_fans_config_v1.md`
- `docs/plans/plan_implementation_api_stats_linux.md`
- `docs/plans/plan_implementation_gpu_monitoring_amd.md`
- `docs/plans/plan_implementation_fans_config.md`

## Regle pratique

En cas de divergence entre texte et implementation:
1. le code backend/firmware fait foi;
2. les tests de contrat backend font foi;
3. les contrats API (`api/docs/*.md`) priment sur les specs haut niveau.
