from pathlib import Path
import os

import pytest
from app import main as main_mod
from app.collectors import fans as fans_collector_mod
from app.models import FansConfigUpdateRequest


def test_get_fans_dashboard_contract_shape() -> None:
    payload = main_mod.get_fans_dashboard().model_dump()

    assert payload["v"] == 1
    assert "ts" in payload
    assert "host" in payload
    assert "fans" in payload
    assert isinstance(payload["fans"], list)

    if payload["fans"]:
        first = payload["fans"][0]
        assert "label" in first
        assert "role" in first
        assert "rpm" in first
        assert "pwm_pct" in first
        assert "pct_fans" in first


def test_get_fans_meta_contract_shape() -> None:
    payload = main_mod.get_fans_meta().model_dump()

    assert payload["v"] == 1
    assert "ts" in payload
    assert "host" in payload
    assert "channels" in payload
    assert "display_labels" in payload
    assert isinstance(payload["channels"], list)

    if payload["channels"]:
        first = payload["channels"][0]
        assert "channel" in first
        assert "hwmon_name" in first
        assert "group" in first
        assert "valid" in first
        assert "mapping" in first


def test_get_fans_config_contract_shape(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "profile" / "fans_mapping.json"
    (hwmon / "hwmon0").mkdir(parents=True, exist_ok=True)
    (hwmon / "hwmon0" / "name").write_text("it8628\n", encoding="utf-8")
    (hwmon / "hwmon0" / "fan1_input").write_text("1200\n", encoding="utf-8")

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = main_mod.get_fans_config().model_dump()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert payload["v"] == 1
    assert payload["mapping_path"] == str(mapping_file)
    assert "unknown" in payload["allowed_roles"]
    assert isinstance(payload["mappings"], list)


def test_put_fans_config_persists_mapping(tmp_path: Path) -> None:
    mapping_file = tmp_path / "profile" / "fans_mapping.json"
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    request = FansConfigUpdateRequest(
        mappings=[
            {
                "label": "CPU",
                "role": "cpu",
                "order": 10,
                "enabled": True,
                "reference_id": "Noctua|NF-A12x25 G2|NF-A12x25 G2 PWM",
                "rpm_min": 0,
                "rpm_max": 1800,
                "match": {"hwmon_name": "it8628", "channel": "fan1", "group": "motherboard"},
            }
        ]
    )
    try:
        payload = main_mod.put_fans_config(request).model_dump()
    finally:
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert payload["v"] == 1
    assert payload["mapping_path"] == str(mapping_file)
    assert "cpu" in payload["allowed_roles"]
    assert payload["mappings"][0]["label"] == "CPU"
    assert payload["mappings"][0]["rpm_max"] == 1800
    assert mapping_file.exists()


def test_get_fans_reference_contract_shape() -> None:
    payload = main_mod.get_fans_reference().model_dump()

    assert payload["v"] == 1
    assert "generated_at" in payload
    assert "count" in payload
    assert "items" in payload
    assert isinstance(payload["items"], list)


def test_fans_config_rejects_invalid_role() -> None:
    with pytest.raises(Exception):
        FansConfigUpdateRequest(
            mappings=[
                {
                    "label": "X",
                    "role": "invalid-role",
                    "order": 1,
                    "enabled": True,
                    "match": {"hwmon_name": "it8628", "channel": "fan1", "group": "motherboard"},
                }
            ]
        )
