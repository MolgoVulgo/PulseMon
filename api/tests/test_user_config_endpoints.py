import os
from pathlib import Path

from app import main as main_mod
from app.models import UserConfigUpdateRequest


def test_get_user_config_defaults_to_empty(tmp_path: Path) -> None:
    db_file = tmp_path / "profile" / "config.db"
    original_env = os.environ.get("STATS_CONFIG_DB_PATH")
    os.environ["STATS_CONFIG_DB_PATH"] = str(db_file)
    try:
        payload = main_mod.get_user_config_v1().model_dump()
    finally:
        if original_env is None:
            os.environ.pop("STATS_CONFIG_DB_PATH", None)
        else:
            os.environ["STATS_CONFIG_DB_PATH"] = original_env

    assert payload["v"] == 1
    assert payload["settings"] == {}


def test_put_user_config_persists_and_reads_back(tmp_path: Path) -> None:
    db_file = tmp_path / "profile" / "config.db"
    original_env = os.environ.get("STATS_CONFIG_DB_PATH")
    os.environ["STATS_CONFIG_DB_PATH"] = str(db_file)
    try:
        request = UserConfigUpdateRequest(
            settings={
                "refresh_hz": 1,
                "fans_view_mode": "meta",
                "show_gpu_graph": True,
            }
        )
        out = main_mod.put_user_config_v1(request).model_dump()
        back = main_mod.get_user_config_v1().model_dump()
    finally:
        if original_env is None:
            os.environ.pop("STATS_CONFIG_DB_PATH", None)
        else:
            os.environ["STATS_CONFIG_DB_PATH"] = original_env

    assert out["v"] == 1
    assert out["settings"]["refresh_hz"] == 1
    assert out["settings"]["fans_view_mode"] == "meta"
    assert back["settings"] == out["settings"]
