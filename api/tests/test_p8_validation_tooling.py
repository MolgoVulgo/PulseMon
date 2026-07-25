from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
RUNNER = PROJECT_ROOT / "tools" / "p8_validate.py"
CHECKLIST = PROJECT_ROOT / "tools" / "p8_checklist.example.json"
DEFAULT_BACKEND_URL = "http://192.168.0.10:8000"


def run_runner(tmp_path: Path, *args: str) -> tuple[subprocess.CompletedProcess[str], dict, Path]:
    output_dir = tmp_path / "result"
    completed = subprocess.run(
        [sys.executable, str(RUNNER), "--output-dir", str(output_dir), *args],
        cwd=PROJECT_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    report_path = output_dir / "report.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    return completed, report, report_path


def test_runner_compiles() -> None:
    completed = subprocess.run(
        [sys.executable, "-m", "py_compile", str(RUNNER)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr


def test_dry_run_build_plan_never_uploads_without_flash(tmp_path: Path) -> None:
    completed, report, _ = run_runner(tmp_path, "--dry-run", "--skip-backend-tests")
    assert completed.returncode == 0, completed.stderr
    commands = [step["command"] for step in report["steps"]]
    assert ["pio", "run", "-e", "pulsmon-esp32s3-display"] in commands
    assert ["pio", "run", "-e", "pulsmon-esp32s3-display-dev"] in commands
    assert not any("upload" in command for command in commands)


def test_dry_run_upload_requires_explicit_flash(tmp_path: Path) -> None:
    completed, report, _ = run_runner(
        tmp_path,
        "--dry-run",
        "--skip-backend-tests",
        "--skip-builds",
        "--flash",
        "--port",
        "/dev/ttyACM0",
    )
    assert completed.returncode == 0, completed.stderr
    commands = [step["command"] for step in report["steps"]]
    assert any("upload" in command for command in commands)


def test_codex_full_profile_plans_complete_pipeline(tmp_path: Path) -> None:
    completed, report, _ = run_runner(
        tmp_path,
        "--codex-full",
        "--dry-run",
        "--port",
        "/dev/ttyACM0",
    )
    assert completed.returncode == 0, completed.stderr
    assert report["overall_status"] == "partial"
    assert report["arguments"]["codex_full"] is True
    assert report["arguments"]["flash"] is True
    assert report["arguments"]["flash_env"] == "pulsmon-esp32s3-display"
    assert report["arguments"]["monitor_seconds"] == 180
    assert report["arguments"]["backend_url"] == DEFAULT_BACKEND_URL

    commands = [step["command"] for step in report["steps"]]
    assert ["pio", "run", "-e", "pulsmon-esp32s3-display"] in commands
    assert ["pio", "run", "-e", "pulsmon-esp32s3-display-dev"] in commands
    assert [
        "pio",
        "run",
        "-e",
        "pulsmon-esp32s3-display",
        "-t",
        "upload",
        "--upload-port",
        "/dev/ttyACM0",
    ] in commands
    assert ["pio", "device", "monitor", "--port", "/dev/ttyACM0", "--baud", "115200"] in commands
    assert {item["url"] for item in report["api_checks"]} == {
        DEFAULT_BACKEND_URL + "/api/v1/health",
        DEFAULT_BACKEND_URL + "/api/v1/dashboard",
        DEFAULT_BACKEND_URL + "/api/v1/gpu/dashboard",
    }
    assert Path(report["checklist_file"]).is_file()


def test_resume_report_preserves_automated_evidence(tmp_path: Path) -> None:
    completed, initial, report_path = run_runner(
        tmp_path,
        "--codex-full",
        "--dry-run",
        "--port",
        "/dev/ttyACM0",
    )
    assert completed.returncode == 0, completed.stderr

    checklist = json.loads(CHECKLIST.read_text(encoding="utf-8"))
    for item in checklist.values():
        item["status"] = "pass"
        item["note"] = "validated on hardware"
    checklist_path = tmp_path / "hardware-pass.json"
    checklist_path.write_text(json.dumps(checklist), encoding="utf-8")

    finalized = subprocess.run(
        [
            sys.executable,
            str(RUNNER),
            "--resume-report",
            str(report_path),
            "--checklist-file",
            str(checklist_path),
            "--require-hardware",
            "--output-dir",
            str(report_path.parent),
        ],
        cwd=PROJECT_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    assert finalized.returncode == 0, finalized.stderr
    final_report = json.loads(report_path.read_text(encoding="utf-8"))
    assert final_report["overall_status"] == "pass"
    assert final_report["steps"] == initial["steps"]
    assert final_report["api_checks"] == initial["api_checks"]
    assert all(item["status"] == "pass" for item in final_report["manual_checks"].values())


def test_checklist_contract_is_complete() -> None:
    data = json.loads(CHECKLIST.read_text(encoding="utf-8"))
    assert set(data) == {
        "metrics_visible",
        "endpoint_reload",
        "local_trigger",
        "sta_preserved",
        "portal_ap_only",
        "manual_timeout",
        "backend_recovery",
        "wifi_recovery",
        "nvs_persistence",
        "weather_news_tls",
    }
    assert all(item["status"] == "pending" for item in data.values())


def test_documentation_exposes_codex_full_workflow() -> None:
    english = (PROJECT_ROOT / "docs" / "p8-validation.md").read_text(encoding="utf-8")
    french = (PROJECT_ROOT / "docs" / "fr" / "p8-validation.md").read_text(encoding="utf-8")
    codex = (PROJECT_ROOT / "codex-patch-mode.md").read_text(encoding="utf-8")
    for text in (english, french, codex):
        assert "--codex-full" in text
        assert DEFAULT_BACKEND_URL in text
        assert "--resume-report" in text
        assert "--require-hardware" in text
        assert "pulsmon-esp32s3-display-dev" in text
    assert "DOIT exécuter" in codex
