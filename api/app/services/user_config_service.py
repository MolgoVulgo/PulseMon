from __future__ import annotations

from app.store.config_db import ConfigDb


def get_user_config() -> dict[str, object]:
    settings = ConfigDb().fetch_user_settings()
    return {"settings": settings}


def save_user_config(payload: dict[str, object]) -> dict[str, object]:
    raw = payload.get("settings")
    settings = raw if isinstance(raw, dict) else {}
    ConfigDb().replace_user_settings(settings)
    return {"settings": settings}


def get_db_data_view() -> dict[str, object]:
    db = ConfigDb()
    return {
        "db_path": str(db.path),
        "fan_mappings": db.fetch_fan_mappings(),
        "user_settings": db.fetch_user_settings(),
    }


def list_db_fans(*, include_deleted: bool) -> dict[str, object]:
    db = ConfigDb()
    rows = db.fetch_fan_mappings(include_deleted=include_deleted, with_ids=True)
    items = [_db_fan_row_payload(row) for row in rows]
    return {"items": items}


def create_db_fan(mapping: dict[str, object]) -> dict[str, object]:
    row = ConfigDb().upsert_fan_mapping(mapping)
    return _db_fan_row_payload(row)


def update_db_fan(fan_id: int, mapping: dict[str, object]) -> dict[str, object]:
    row = ConfigDb().upsert_fan_mapping(mapping, row_id=fan_id)
    return _db_fan_row_payload(row)


def soft_delete_db_fan(fan_id: int) -> dict[str, object]:
    row = ConfigDb().soft_delete_fan_mapping(fan_id)
    return _db_fan_row_payload(row)


def restore_db_fan(fan_id: int) -> dict[str, object]:
    row = ConfigDb().restore_fan_mapping(fan_id)
    return _db_fan_row_payload(row)


def hard_delete_db_fan(fan_id: int) -> dict[str, object]:
    ConfigDb().hard_delete_fan_mapping(fan_id)
    return {"deleted_id": fan_id}


def _db_fan_row_payload(row: dict[str, object]) -> dict[str, object]:
    return {
        "id": int(row.get("id", 0)),
        "deleted": bool(row.get("deleted", False)),
        "mapping": {
            "label": row.get("label"),
            "role": row.get("role"),
            "order": row.get("order"),
            "enabled": row.get("enabled"),
            "reference_id": row.get("reference_id"),
            "rpm_min": row.get("rpm_min"),
            "rpm_max": row.get("rpm_max"),
            "match": row.get("match"),
        },
    }
