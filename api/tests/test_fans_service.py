from pathlib import Path
import os

from app.collectors import fans as fans_collector_mod
from app.services import fans_service as fans_service_mod


def _write(path: Path, data: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(data, encoding="utf-8")


def test_build_fans_dashboard_fallback_scans_without_mapping_file(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "1300\n")
    _write(hwmon / "hwmon0" / "fan2_input", "0\n")

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_loader = fans_service_mod._load_mappings
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    fans_service_mod._load_mappings = lambda channels: []
    try:
        payload = fans_service_mod.build_fans_dashboard().model_dump()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        fans_service_mod._load_mappings = original_loader

    assert payload["v"] == 1
    assert len(payload["fans"]) == 1
    assert payload["fans"][0]["label"] == "fan1"
    assert payload["fans"][0]["role"] == "unknown"
    assert payload["fans"][0]["rpm"] == 1300


def test_bootstrap_mapping_file_is_created_on_first_run(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "config" / "fans_mapping.json"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "1200\n")

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = fans_service_mod.build_fans_dashboard().model_dump()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert mapping_file.exists()
    assert payload["fans"]
    assert payload["fans"][0]["label"] == "fan1"
    assert payload["fans"][0]["role"] == "unknown"
    assert payload["fans"][0]["rpm"] == 1200


def test_bootstrap_mapping_disables_off_fan(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "config" / "fans_mapping.json"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "0\n")

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = fans_service_mod.build_fans_dashboard().model_dump()
        raw = mapping_file.read_text(encoding="utf-8")
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert payload["fans"] == []
    assert '"enabled": false' in raw


def test_get_fans_mapping_config_bootstraps_user_file(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "profile" / "fans_mapping.json"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "1100\n")

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = fans_service_mod.get_fans_mapping_config()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert mapping_file.exists()
    assert payload["mapping_path"] == str(mapping_file)
    assert isinstance(payload["mappings"], list)
    assert payload["mappings"][0]["match"]["channel"] == "fan1"


def test_save_fans_mapping_config_writes_file(tmp_path: Path) -> None:
    mapping_file = tmp_path / "profile" / "fans_mapping.json"
    payload = {
        "mappings": [
            {
                "label": "CPU",
                "role": "cpu",
                "order": 10,
                "enabled": True,
                "match": {"hwmon_name": "it8628", "channel": "fan1", "group": "motherboard"},
            }
        ]
    }
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        out = fans_service_mod.save_fans_mapping_config(payload)
    finally:
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert mapping_file.exists()
    assert out["mapping_path"] == str(mapping_file)
    assert out["mappings"][0]["label"] == "CPU"


def test_build_fans_dashboard_computes_pct_fans_from_config_bounds(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "config" / "fans_mapping.json"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "1024\n")
    _write(
        mapping_file,
        """
{
  "mappings": [
    {
      "label": "CPU",
      "role": "cpu",
      "order": 1,
      "enabled": true,
      "rpm_min": 300,
      "rpm_max": 1800,
      "match": { "hwmon_name": "it8628", "channel": "fan1", "group": "motherboard" }
    }
  ]
}
""".strip()
        + "\n",
    )

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = fans_service_mod.build_fans_dashboard().model_dump()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert payload["fans"][0]["pct_fans"] == 48


def test_build_fans_dashboard_sets_pct_fans_null_when_bounds_invalid(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "config" / "fans_mapping.json"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "1200\n")
    _write(
        mapping_file,
        """
{
  "mappings": [
    {
      "label": "CPU",
      "role": "cpu",
      "order": 1,
      "enabled": true,
      "rpm_min": 1200,
      "rpm_max": 1200,
      "match": { "hwmon_name": "it8628", "channel": "fan1", "group": "motherboard" }
    }
  ]
}
""".strip()
        + "\n",
    )

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = fans_service_mod.build_fans_dashboard().model_dump()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert payload["fans"][0]["pct_fans"] is None


def test_build_fans_dashboard_uses_zero_when_rpm_min_missing(tmp_path: Path) -> None:
    hwmon = tmp_path / "hwmon"
    mapping_file = tmp_path / "config" / "fans_mapping.json"
    _write(hwmon / "hwmon0" / "name", "it8628\n")
    _write(hwmon / "hwmon0" / "fan1_input", "559\n")
    _write(
        mapping_file,
        """
{
  "mappings": [
    {
      "label": "Rear",
      "role": "case",
      "order": 1,
      "enabled": true,
      "rpm_min": null,
      "rpm_max": 1200,
      "match": { "hwmon_name": "it8628", "channel": "fan1", "group": "motherboard" }
    }
  ]
}
""".strip()
        + "\n",
    )

    original_hwmon = fans_collector_mod.HWMON_CLASS_PATH
    original_env = os.environ.get("STATS_FANS_MAPPING_FILE")
    fans_collector_mod.HWMON_CLASS_PATH = hwmon
    os.environ["STATS_FANS_MAPPING_FILE"] = str(mapping_file)
    try:
        payload = fans_service_mod.build_fans_dashboard().model_dump()
    finally:
        fans_collector_mod.HWMON_CLASS_PATH = original_hwmon
        if original_env is None:
            os.environ.pop("STATS_FANS_MAPPING_FILE", None)
        else:
            os.environ["STATS_FANS_MAPPING_FILE"] = original_env

    assert payload["fans"][0]["pct_fans"] == 47
