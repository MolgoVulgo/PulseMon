from __future__ import annotations

import json
import os
from pathlib import Path
import sqlite3


_DEFAULT_DB_PATH = Path.home() / ".config" / "pulsemon" / "config.db"
_TMP_DB_PATH = Path("/tmp/pulsemon/config.db")


def resolve_config_db_path() -> Path:
    configured = os.getenv("STATS_CONFIG_DB_PATH")
    if configured:
        return Path(configured)
    if _ensure_parent_writable(_DEFAULT_DB_PATH):
        return _DEFAULT_DB_PATH
    return _TMP_DB_PATH


def config_db_uri() -> str:
    return f"sqlite://{resolve_config_db_path()}"


def _ensure_parent_writable(path: Path) -> bool:
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
    except OSError:
        return False
    return os.access(path.parent, os.W_OK)


class ConfigDb:
    def __init__(self, path: Path | None = None) -> None:
        self._path = path if path is not None else resolve_config_db_path()

    @property
    def path(self) -> Path:
        return self._path

    def ensure_schema(self) -> None:
        self._path.parent.mkdir(parents=True, exist_ok=True)
        with sqlite3.connect(self._path) as conn:
            conn.execute("PRAGMA journal_mode=WAL;")
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS schema_migrations (
                    version INTEGER PRIMARY KEY,
                    applied_at TEXT NOT NULL
                )
                """
            )
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS fan_mappings (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    label TEXT NOT NULL,
                    role TEXT NOT NULL,
                    order_idx INTEGER NOT NULL,
                    enabled INTEGER NOT NULL,
                    reference_id TEXT,
                    rpm_min INTEGER,
                    rpm_max INTEGER,
                    hwmon_name TEXT,
                    channel TEXT,
                    group_name TEXT,
                    hwmon_path_contains TEXT,
                    deleted_at TEXT,
                    updated_at TEXT NOT NULL
                )
                """
            )
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS user_settings (
                    key TEXT PRIMARY KEY,
                    value_json TEXT NOT NULL,
                    updated_at TEXT NOT NULL
                )
                """
            )
            _ensure_column(conn, "fan_mappings", "deleted_at", "TEXT")
            conn.commit()

    def fetch_fan_mappings(
        self,
        *,
        include_deleted: bool = False,
        with_ids: bool = False,
    ) -> list[dict[str, object]]:
        self.ensure_schema()
        where_clause = "" if include_deleted else "WHERE deleted_at IS NULL"
        with sqlite3.connect(self._path) as conn:
            conn.row_factory = sqlite3.Row
            rows = conn.execute(
                """
                SELECT
                    id,
                    label,
                    role,
                    order_idx,
                    enabled,
                    reference_id,
                    rpm_min,
                    rpm_max,
                    hwmon_name,
                    channel,
                    group_name,
                    hwmon_path_contains,
                    deleted_at
                FROM fan_mappings
                """
                + where_clause
                + """
                ORDER BY order_idx ASC, id ASC
                """
            ).fetchall()

        payload: list[dict[str, object]] = []
        for row in rows:
            payload.append(
                {
                    "label": row["label"],
                    "role": row["role"],
                    "order": int(row["order_idx"]),
                    "enabled": bool(row["enabled"]),
                    "reference_id": row["reference_id"],
                    "rpm_min": row["rpm_min"],
                    "rpm_max": row["rpm_max"],
                    "match": {
                        "hwmon_name": row["hwmon_name"],
                        "channel": row["channel"],
                        "group": row["group_name"],
                        "hwmon_path_contains": row["hwmon_path_contains"],
                    },
                }
            )
            if with_ids:
                payload[-1]["id"] = int(row["id"])
                payload[-1]["deleted"] = row["deleted_at"] is not None
        return payload

    def replace_fan_mappings(self, rows: list[dict[str, object]]) -> None:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            conn.execute("BEGIN")
            conn.execute("DELETE FROM fan_mappings")
            for row in rows:
                match = row.get("match")
                if not isinstance(match, dict):
                    match = {}
                conn.execute(
                    """
                    INSERT INTO fan_mappings (
                        label,
                        role,
                        order_idx,
                        enabled,
                        reference_id,
                        rpm_min,
                        rpm_max,
                        hwmon_name,
                        channel,
                        group_name,
                        hwmon_path_contains,
                        deleted_at,
                        updated_at
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NULL, datetime('now'))
                    """,
                    (
                        row.get("label"),
                        row.get("role"),
                        int(row.get("order", 100)),
                        1 if bool(row.get("enabled", True)) else 0,
                        row.get("reference_id"),
                        row.get("rpm_min"),
                        row.get("rpm_max"),
                        match.get("hwmon_name"),
                        match.get("channel"),
                        match.get("group"),
                        match.get("hwmon_path_contains"),
                    ),
                )
            conn.commit()

    def upsert_fan_mapping(self, row: dict[str, object], row_id: int | None = None) -> dict[str, object]:
        self.ensure_schema()
        match = row.get("match")
        if not isinstance(match, dict):
            match = {}
        with sqlite3.connect(self._path) as conn:
            conn.row_factory = sqlite3.Row
            if row_id is None:
                cur = conn.execute(
                    """
                    INSERT INTO fan_mappings (
                        label,
                        role,
                        order_idx,
                        enabled,
                        reference_id,
                        rpm_min,
                        rpm_max,
                        hwmon_name,
                        channel,
                        group_name,
                        hwmon_path_contains,
                        deleted_at,
                        updated_at
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NULL, datetime('now'))
                    """,
                    (
                        row.get("label"),
                        row.get("role"),
                        int(row.get("order", 100)),
                        1 if bool(row.get("enabled", True)) else 0,
                        row.get("reference_id"),
                        row.get("rpm_min"),
                        row.get("rpm_max"),
                        match.get("hwmon_name"),
                        match.get("channel"),
                        match.get("group"),
                        match.get("hwmon_path_contains"),
                    ),
                )
                row_id = int(cur.lastrowid)
            else:
                cur = conn.execute(
                    """
                    UPDATE fan_mappings
                    SET
                        label = ?,
                        role = ?,
                        order_idx = ?,
                        enabled = ?,
                        reference_id = ?,
                        rpm_min = ?,
                        rpm_max = ?,
                        hwmon_name = ?,
                        channel = ?,
                        group_name = ?,
                        hwmon_path_contains = ?,
                        updated_at = datetime('now')
                    WHERE id = ?
                    """,
                    (
                        row.get("label"),
                        row.get("role"),
                        int(row.get("order", 100)),
                        1 if bool(row.get("enabled", True)) else 0,
                        row.get("reference_id"),
                        row.get("rpm_min"),
                        row.get("rpm_max"),
                        match.get("hwmon_name"),
                        match.get("channel"),
                        match.get("group"),
                        match.get("hwmon_path_contains"),
                        int(row_id),
                    ),
                )
                if cur.rowcount == 0:
                    raise KeyError(f"fan mapping id not found: {row_id}")
            conn.commit()
        return self.fetch_fan_mapping_by_id(int(row_id), include_deleted=True)

    def fetch_fan_mapping_by_id(self, row_id: int, *, include_deleted: bool = False) -> dict[str, object]:
        rows = self.fetch_fan_mappings(include_deleted=include_deleted, with_ids=True)
        for row in rows:
            if int(row.get("id", -1)) == row_id:
                return row
        raise KeyError(f"fan mapping id not found: {row_id}")

    def soft_delete_fan_mapping(self, row_id: int) -> dict[str, object]:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            cur = conn.execute(
                """
                UPDATE fan_mappings
                SET enabled = 0, deleted_at = datetime('now'), updated_at = datetime('now')
                WHERE id = ?
                """,
                (int(row_id),),
            )
            if cur.rowcount == 0:
                raise KeyError(f"fan mapping id not found: {row_id}")
            conn.commit()
        return self.fetch_fan_mapping_by_id(int(row_id), include_deleted=True)

    def restore_fan_mapping(self, row_id: int) -> dict[str, object]:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            cur = conn.execute(
                """
                UPDATE fan_mappings
                SET deleted_at = NULL, enabled = 1, updated_at = datetime('now')
                WHERE id = ?
                """,
                (int(row_id),),
            )
            if cur.rowcount == 0:
                raise KeyError(f"fan mapping id not found: {row_id}")
            conn.commit()
        return self.fetch_fan_mapping_by_id(int(row_id), include_deleted=True)

    def hard_delete_fan_mapping(self, row_id: int) -> None:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            cur = conn.execute("DELETE FROM fan_mappings WHERE id = ?", (int(row_id),))
            if cur.rowcount == 0:
                raise KeyError(f"fan mapping id not found: {row_id}")
            conn.commit()

    def import_fan_mappings_if_empty(self, rows: list[dict[str, object]]) -> bool:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            count = conn.execute("SELECT COUNT(*) FROM fan_mappings").fetchone()[0]
        if int(count) > 0:
            return False
        self.replace_fan_mappings(rows)
        return True

    def fetch_user_settings(self) -> dict[str, object]:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            conn.row_factory = sqlite3.Row
            rows = conn.execute("SELECT key, value_json FROM user_settings ORDER BY key ASC").fetchall()

        out: dict[str, object] = {}
        for row in rows:
            try:
                out[row["key"]] = json.loads(row["value_json"])
            except json.JSONDecodeError:
                out[row["key"]] = row["value_json"]
        return out

    def replace_user_settings(self, settings: dict[str, object]) -> None:
        self.ensure_schema()
        with sqlite3.connect(self._path) as conn:
            conn.execute("BEGIN")
            conn.execute("DELETE FROM user_settings")
            for key, value in settings.items():
                conn.execute(
                    "INSERT INTO user_settings (key, value_json, updated_at) VALUES (?, ?, datetime('now'))",
                    (str(key), json.dumps(value, separators=(",", ":"), ensure_ascii=True)),
                )
            conn.commit()


def _ensure_column(conn: sqlite3.Connection, table: str, column: str, column_type: str) -> None:
    rows = conn.execute(f"PRAGMA table_info({table})").fetchall()
    for row in rows:
        if len(row) > 1 and row[1] == column:
            return
    conn.execute(f"ALTER TABLE {table} ADD COLUMN {column} {column_type}")
