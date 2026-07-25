from __future__ import annotations

from pathlib import Path
import re

import pytest

from app.config import (
    BACKEND_ENV_VARS,
    DEFAULT_GPU_TEMP_LABEL_PRIORITY,
    ConfigError,
    load_config,
    main,
)


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def _clear_backend_env(monkeypatch: pytest.MonkeyPatch) -> None:
    for name in BACKEND_ENV_VARS:
        monkeypatch.delenv(name, raising=False)


def test_backend_environment_contract_contains_exactly_20_variables() -> None:
    assert len(BACKEND_ENV_VARS) == 20
    assert len(set(BACKEND_ENV_VARS)) == 20


def test_load_config_defaults(monkeypatch: pytest.MonkeyPatch) -> None:
    _clear_backend_env(monkeypatch)

    config = load_config()

    assert config.bind_host == "0.0.0.0"
    assert config.bind_port == 8000
    assert config.sample_interval_s == 0.1
    assert config.publish_interval_s == 0.5
    assert config.history_capacity == 600
    assert config.api_key is None
    assert config.api_key_header == "X-API-Key"
    assert config.log_level == "INFO"
    assert config.diagnostics_enabled is False
    assert config.display_ema_alpha == 0.25
    assert config.gpu_pci_slot is None
    assert config.gpu_temp_label_priority == DEFAULT_GPU_TEMP_LABEL_PRIORITY


def test_load_config_accepts_and_normalizes_valid_values(monkeypatch: pytest.MonkeyPatch) -> None:
    _clear_backend_env(monkeypatch)
    values = {
        "STATS_BIND_HOST": "localhost",
        "STATS_BIND_PORT": "65535",
        "STATS_SAMPLE_INTERVAL_S": "0.01",
        "STATS_PUBLISH_INTERVAL_S": "3600",
        "STATS_HISTORY_CAPACITY": "1000000",
        "STATS_API_KEY": "local-secret",
        "STATS_API_KEY_HEADER": "X-PulseMon-Key",
        "STATS_LOG_LEVEL": "warn",
        "STATS_DIAGNOSTICS": "yes",
        "STATS_DIAG_RAW_CAPTURE": "on",
        "STATS_DIAG_RAW_HZ": "1000",
        "STATS_DIAG_RAW_DURATION_S": "86400",
        "STATS_DIAG_RAW_LOG_PATH": "/tmp/raw.jsonl",
        "STATS_DIAG_COMPARE_CAPTURE": "false",
        "STATS_DIAG_COMPARE_HZ": "0.1",
        "STATS_DIAG_COMPARE_DURATION_S": "1",
        "STATS_DIAG_COMPARE_LOG_PATH": "diagnostics/compare.jsonl",
        "STATS_DISPLAY_EMA_ALPHA": "1",
        "STATS_GPU_PCI_SLOT": "0000:0A:00.0",
        "STATS_GPU_TEMP_LABEL_PRIORITY": "junction, edge",
    }
    for name, value in values.items():
        monkeypatch.setenv(name, value)

    config = load_config()

    assert config.bind_host == "localhost"
    assert config.bind_port == 65_535
    assert config.log_level == "WARNING"
    assert config.diagnostics_enabled is True
    assert config.diagnostics_raw_capture is True
    assert config.diagnostics_compare_capture is False
    assert config.gpu_pci_slot == "0000:0a:00.0"
    assert config.gpu_temp_label_priority == ("junction", "edge", "unknown")


@pytest.mark.parametrize(
    ("name", "value", "message"),
    [
        ("STATS_BIND_HOST", "bad host", "valid IP address or hostname"),
        ("STATS_BIND_PORT", "0", "[1, 65535]"),
        ("STATS_BIND_PORT", "65536", "[1, 65535]"),
        ("STATS_BIND_PORT", "eight", "integer"),
        ("STATS_SAMPLE_INTERVAL_S", "0", "[0.01, 60.0]"),
        ("STATS_SAMPLE_INTERVAL_S", "nan", "finite number"),
        ("STATS_PUBLISH_INTERVAL_S", "3600.1", "[0.01, 3600.0]"),
        ("STATS_HISTORY_CAPACITY", "0", "[1, 1000000]"),
        ("STATS_API_KEY_HEADER", "X Bad", "HTTP header token"),
        ("STATS_LOG_LEVEL", "VERBOSE", "must be one of"),
        ("STATS_DIAGNOSTICS", "maybe", "1/0"),
        ("STATS_DIAG_RAW_CAPTURE", "", "1/0"),
        ("STATS_DIAG_RAW_HZ", "0", "[0.1, 1000.0]"),
        ("STATS_DIAG_RAW_DURATION_S", "86401", "[1, 86400]"),
        ("STATS_DIAG_RAW_LOG_PATH", " ", "filesystem path"),
        ("STATS_DIAG_COMPARE_CAPTURE", "2", "1/0"),
        ("STATS_DIAG_COMPARE_HZ", "inf", "finite number"),
        ("STATS_DIAG_COMPARE_DURATION_S", "0", "[1, 86400]"),
        ("STATS_DIAG_COMPARE_LOG_PATH", "bad\npath", "filesystem path"),
        ("STATS_DISPLAY_EMA_ALPHA", "0", "(0.0, 1.0]"),
        ("STATS_DISPLAY_EMA_ALPHA", "1.01", "(0.0, 1.0]"),
        ("STATS_GPU_PCI_SLOT", "card0", "PCI BDF"),
        ("STATS_GPU_TEMP_LABEL_PRIORITY", "edge,,junction", "invalid temperature label"),
        ("STATS_GPU_TEMP_LABEL_PRIORITY", "edge,EDGE", "duplicate labels"),
    ],
)
def test_load_config_rejects_invalid_values(
    monkeypatch: pytest.MonkeyPatch,
    name: str,
    value: str,
    message: str,
) -> None:
    _clear_backend_env(monkeypatch)
    monkeypatch.setenv(name, value)

    with pytest.raises(ConfigError, match=re.escape(message)):
        load_config()


def test_api_key_error_does_not_disclose_secret(monkeypatch: pytest.MonkeyPatch) -> None:
    _clear_backend_env(monkeypatch)
    secret = "must-not-leak\n"
    monkeypatch.setenv("STATS_API_KEY", secret)

    with pytest.raises(ConfigError) as exc_info:
        load_config()

    assert "must-not-leak" not in str(exc_info.value)


def test_config_preflight_returns_nonzero_without_disclosing_secret(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    _clear_backend_env(monkeypatch)
    monkeypatch.setenv("STATS_API_KEY", "hidden-value\n")

    assert main() == 2
    captured = capsys.readouterr()
    assert "PulseMon backend configuration error" in captured.err
    assert "hidden-value" not in captured.err


def test_documentation_and_packaged_config_match_environment_contract() -> None:
    expected = set(BACKEND_ENV_VARS)
    for relative_path in (
        "docs/configuration.md",
        "docs/fr/configuration.md",
        "pulsemon-api.conf",
    ):
        text = (PROJECT_ROOT / relative_path).read_text(encoding="utf-8")
        found = set(re.findall(r"STATS_[A-Z0-9_]+", text))
        assert found == expected, (relative_path, sorted(expected - found), sorted(found - expected))


def test_python_runtime_has_no_direct_stats_environment_reads_outside_config() -> None:
    offenders: list[str] = []
    app_root = PROJECT_ROOT / "api" / "app"
    for path in app_root.rglob("*.py"):
        if path.name == "config.py":
            continue
        text = path.read_text(encoding="utf-8")
        if re.search(r"os\.(?:getenv|environ).*STATS_", text):
            offenders.append(str(path.relative_to(PROJECT_ROOT)))
    assert offenders == []


def test_packaged_launcher_preflights_only_documented_stats_variables() -> None:
    launcher = (PROJECT_ROOT / "pulsemon-api.sh").read_text(encoding="utf-8")
    assert "python -m app.config" in launcher
    assert "STATS_UVICORN_LOG_LEVEL" not in launcher
    used = set(re.findall(r"STATS_[A-Z0-9_]+", launcher))
    assert used <= set(BACKEND_ENV_VARS)
