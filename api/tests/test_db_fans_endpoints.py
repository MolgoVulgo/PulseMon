import os
from pathlib import Path

from fastapi import HTTPException

from app import main as main_mod
from app.models import DbFanWriteRequest


def _request(label: str, order: int = 100) -> DbFanWriteRequest:
    return DbFanWriteRequest(
        mapping={
            "label": label,
            "role": "unknown",
            "order": order,
            "enabled": True,
            "reference_id": None,
            "rpm_min": None,
            "rpm_max": None,
            "match": {
                "hwmon_name": None,
                "channel": None,
                "group": None,
                "hwmon_path_contains": None,
            },
        }
    )


def test_db_fans_crud_flow(tmp_path: Path) -> None:
    db_file = tmp_path / "profile" / "config.db"
    original_env = os.environ.get("STATS_CONFIG_DB_PATH")
    os.environ["STATS_CONFIG_DB_PATH"] = str(db_file)
    try:
        created = main_mod.post_db_fan(_request("Fan A", order=10)).model_dump()
        fan_id = created["item"]["id"]
        assert created["v"] == 1
        assert created["item"]["mapping"]["label"] == "Fan A"
        assert created["item"]["deleted"] is False

        listed = main_mod.get_db_fans(include_deleted=True).model_dump()
        assert listed["v"] == 1
        assert any(item["id"] == fan_id for item in listed["items"])

        updated = main_mod.put_db_fan(fan_id, _request("Fan A edited", order=20)).model_dump()
        assert updated["item"]["mapping"]["label"] == "Fan A edited"
        assert updated["item"]["mapping"]["order"] == 20

        soft_deleted = main_mod.post_db_fan_soft_delete(fan_id).model_dump()
        assert soft_deleted["item"]["deleted"] is True
        assert soft_deleted["item"]["mapping"]["enabled"] is False

        active_only = main_mod.get_db_fans(include_deleted=False).model_dump()
        assert all(item["id"] != fan_id for item in active_only["items"])

        restored = main_mod.post_db_fan_restore(fan_id).model_dump()
        assert restored["item"]["deleted"] is False
        assert restored["item"]["mapping"]["enabled"] is True

        deleted = main_mod.delete_db_fan(fan_id).model_dump()
        assert deleted["deleted_id"] == fan_id

        listed_after = main_mod.get_db_fans(include_deleted=True).model_dump()
        assert all(item["id"] != fan_id for item in listed_after["items"])
    finally:
        if original_env is None:
            os.environ.pop("STATS_CONFIG_DB_PATH", None)
        else:
            os.environ["STATS_CONFIG_DB_PATH"] = original_env


def test_db_fan_not_found_returns_404(tmp_path: Path) -> None:
    db_file = tmp_path / "profile" / "config.db"
    original_env = os.environ.get("STATS_CONFIG_DB_PATH")
    os.environ["STATS_CONFIG_DB_PATH"] = str(db_file)
    try:
        try:
            main_mod.put_db_fan(999, _request("missing"))
        except HTTPException as exc:
            assert exc.status_code == 404
            assert exc.detail == {"v": 1, "error": "not_found", "field": "fan_id"}
        else:
            raise AssertionError("HTTPException not raised")
    finally:
        if original_env is None:
            os.environ.pop("STATS_CONFIG_DB_PATH", None)
        else:
            os.environ["STATS_CONFIG_DB_PATH"] = original_env
