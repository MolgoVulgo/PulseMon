#!/usr/bin/env python3
"""PulseMon P8 validation runner.

The runner provides two controlled profiles:
- the normal profile builds and tests without implicit hardware operations;
- --codex-full performs the explicitly authorized P8 pipeline: backend tests,
  release/debug builds, release flash, bounded serial monitor and backend API checks;
- physical observations are finalized from the same report through --resume-report;
- secrets are read from environment variables and are never written to reports.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.request
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Iterable

PROJECT_ROOT = Path(__file__).resolve().parents[1]
API_DIR = PROJECT_ROOT / "api"
ESP_DIR = PROJECT_ROOT / "esp"
RELEASE_ENV = "pulsmon-esp32s3-display"
DEBUG_ENV = "pulsmon-esp32s3-display-dev"
DEFAULT_BACKEND_URL = "http://192.168.0.10:8000"
CODEX_MONITOR_SECONDS = 180
DEFAULT_ENDPOINTS = (
    "/api/v1/health",
    "/api/v1/dashboard",
    "/api/v1/gpu/dashboard",
)
MANUAL_CHECKS = (
    ("metrics_visible", "CPU, memory and GPU metrics are visible on the active screens."),
    ("endpoint_reload", "Changing backend host/port in the portal is applied without reboot."),
    ("local_trigger", "A five-second top-left hold opens PulseMon-Setup."),
    ("sta_preserved", "The station connection remains active during the manual AP window."),
    ("portal_ap_only", "The portal is reachable through the setup AP and not exposed as a permanent STA-LAN service."),
    ("manual_timeout", "The manual setup AP closes after approximately ten minutes when STA remains connected."),
    ("backend_recovery", "Displayed data survives backend loss and refreshes after backend recovery."),
    ("wifi_recovery", "The firmware recovers after Wi-Fi loss and reconnects without clearing NVS."),
    ("nvs_persistence", "Backend endpoint and service settings persist after reboot."),
    ("weather_news_tls", "OpenWeather and GNews update successfully over TLS."),
)
LOG_PATTERNS = {
    "local_config_requested": "local configuration mode requested",
    "setup_ap_active": "config portal ap active",
    "config_server_started": "wifi config server started",
    "captive_dns_started": "captive dns started",
    "manual_window_expired": "manual configuration window expired",
    "config_server_stopped": "wifi config server stopped",
    "backend_online": "backend online",
    "backend_offline": "backend offline",
}
ALLOWED_CHECKLIST_STATUSES = {"pass", "fail", "skip", "pending"}


@dataclass
class StepResult:
    name: str
    status: str
    command: list[str] = field(default_factory=list)
    cwd: str | None = None
    returncode: int | None = None
    duration_s: float = 0.0
    stdout_file: str | None = None
    stderr_file: str | None = None
    detail: str = ""


@dataclass
class ValidationReport:
    started_at: str
    finished_at: str = ""
    project_root: str = str(PROJECT_ROOT)
    arguments: dict[str, Any] = field(default_factory=dict)
    devices: list[dict[str, Any]] = field(default_factory=list)
    steps: list[StepResult] = field(default_factory=list)
    api_checks: list[dict[str, Any]] = field(default_factory=list)
    log_evidence: dict[str, bool] = field(default_factory=dict)
    manual_checks: dict[str, dict[str, str]] = field(default_factory=dict)
    checklist_file: str | None = None
    overall_status: str = "running"


def now_iso() -> str:
    return dt.datetime.now(dt.timezone.utc).astimezone().isoformat(timespec="seconds")


def sanitize_args(namespace: argparse.Namespace) -> dict[str, Any]:
    data: dict[str, Any] = {}
    for key, value in vars(namespace).items():
        if isinstance(value, Path):
            data[key] = str(value)
        else:
            data[key] = value
    # Only environment variable names are accepted; secret values are never arguments.
    return data


def command_exists(name: str) -> bool:
    return shutil.which(name) is not None


def choose_python() -> str:
    venv_python = API_DIR / ".venv" / "bin" / "python"
    if venv_python.is_file() and os.access(venv_python, os.X_OK):
        return str(venv_python)
    return sys.executable


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def run_command(
    *,
    name: str,
    command: list[str],
    cwd: Path,
    output_dir: Path,
    timeout: int | None = None,
    dry_run: bool = False,
) -> StepResult:
    safe_name = "".join(ch if ch.isalnum() or ch in "-_" else "_" for ch in name)
    stdout_path = output_dir / f"{safe_name}.stdout.log"
    stderr_path = output_dir / f"{safe_name}.stderr.log"
    started = time.monotonic()

    if dry_run:
        write_text(stdout_path, "DRY RUN: " + " ".join(command) + "\n")
        write_text(stderr_path, "")
        return StepResult(
            name=name,
            status="dry-run",
            command=command,
            cwd=str(cwd),
            returncode=0,
            duration_s=0.0,
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail="Command not executed.",
        )

    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
            check=False,
        )
        write_text(stdout_path, completed.stdout)
        write_text(stderr_path, completed.stderr)
        return StepResult(
            name=name,
            status="pass" if completed.returncode == 0 else "fail",
            command=command,
            cwd=str(cwd),
            returncode=completed.returncode,
            duration_s=round(time.monotonic() - started, 3),
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail="",
        )
    except subprocess.TimeoutExpired as exc:
        write_text(stdout_path, exc.stdout or "")
        write_text(stderr_path, exc.stderr or "")
        return StepResult(
            name=name,
            status="fail",
            command=command,
            cwd=str(cwd),
            returncode=None,
            duration_s=round(time.monotonic() - started, 3),
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail=f"Timeout after {timeout} seconds.",
        )
    except OSError as exc:
        write_text(stdout_path, "")
        write_text(stderr_path, str(exc) + "\n")
        return StepResult(
            name=name,
            status="blocked",
            command=command,
            cwd=str(cwd),
            returncode=None,
            duration_s=round(time.monotonic() - started, 3),
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail=str(exc),
        )


def detect_devices(*, dry_run: bool = False) -> tuple[list[dict[str, Any]], StepResult]:
    if dry_run:
        return [], StepResult(name="device-detection", status="dry-run", detail="Device detection not executed.")
    command = ["pio", "device", "list", "--json-output"]
    started = time.monotonic()
    try:
        completed = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    except OSError as exc:
        return [], StepResult(name="device-detection", status="blocked", command=command, detail=str(exc))
    devices: list[dict[str, Any]] = []
    detail = completed.stderr.strip()
    if completed.returncode == 0:
        try:
            parsed = json.loads(completed.stdout or "[]")
            if isinstance(parsed, list):
                devices = [item for item in parsed if isinstance(item, dict)]
            else:
                detail = "PlatformIO returned a non-list JSON payload."
        except json.JSONDecodeError as exc:
            detail = f"Invalid JSON from PlatformIO: {exc}"
    status = "pass" if completed.returncode == 0 else "fail"
    return devices, StepResult(
        name="device-detection",
        status=status,
        command=command,
        returncode=completed.returncode,
        duration_s=round(time.monotonic() - started, 3),
        detail=detail,
    )


def resolve_port(explicit_port: str | None, devices: list[dict[str, Any]]) -> str | None:
    if explicit_port:
        return explicit_port
    ports = [str(item.get("port")) for item in devices if item.get("port")]
    if len(ports) == 1:
        return ports[0]
    return None


def capture_monitor(
    *, port: str, seconds: int, output_dir: Path, dry_run: bool
) -> StepResult:
    name = "serial-monitor"
    command = ["pio", "device", "monitor", "--port", port, "--baud", "115200"]
    stdout_path = output_dir / "serial-monitor.stdout.log"
    stderr_path = output_dir / "serial-monitor.stderr.log"
    if dry_run:
        write_text(stdout_path, "DRY RUN: " + " ".join(command) + "\n")
        write_text(stderr_path, "")
        return StepResult(
            name=name,
            status="dry-run",
            command=command,
            cwd=str(ESP_DIR),
            returncode=0,
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail=f"Monitor would run for {seconds} seconds.",
        )

    started = time.monotonic()
    try:
        process = subprocess.Popen(
            command,
            cwd=ESP_DIR,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        try:
            stdout, stderr = process.communicate(timeout=seconds)
            detail = "Monitor exited before timeout."
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                stdout, stderr = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                stdout, stderr = process.communicate()
            detail = f"Monitor capture completed after {seconds} seconds."
        write_text(stdout_path, stdout)
        write_text(stderr_path, stderr)
        returncode = process.returncode
        # A timeout-driven SIGTERM is expected and should not fail the capture.
        status = "pass" if returncode in (0, -15, 143) else "fail"
        return StepResult(
            name=name,
            status=status,
            command=command,
            cwd=str(ESP_DIR),
            returncode=returncode,
            duration_s=round(time.monotonic() - started, 3),
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail=detail,
        )
    except OSError as exc:
        write_text(stdout_path, "")
        write_text(stderr_path, str(exc) + "\n")
        return StepResult(
            name=name,
            status="blocked",
            command=command,
            cwd=str(ESP_DIR),
            duration_s=round(time.monotonic() - started, 3),
            stdout_file=str(stdout_path),
            stderr_file=str(stderr_path),
            detail=str(exc),
        )


def inspect_monitor_log(step: StepResult) -> dict[str, bool]:
    if not step.stdout_file:
        return {name: False for name in LOG_PATTERNS}
    path = Path(step.stdout_file)
    text = path.read_text(encoding="utf-8", errors="replace").lower() if path.exists() else ""
    return {name: pattern.lower() in text for name, pattern in LOG_PATTERNS.items()}


def api_check(
    *, base_url: str, endpoint: str, timeout: float, header_name: str, api_key: str | None
) -> dict[str, Any]:
    url = base_url.rstrip("/") + endpoint
    headers = {"Accept": "application/json"}
    if api_key:
        headers[header_name] = api_key
    request = urllib.request.Request(url, headers=headers, method="GET")
    started = time.monotonic()
    result: dict[str, Any] = {
        "endpoint": endpoint,
        "url": url,
        "status": "fail",
        "http_status": None,
        "duration_s": 0.0,
        "detail": "",
    }
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = json.loads(response.read().decode("utf-8"))
            result["http_status"] = response.status
            if response.status != 200:
                result["detail"] = f"Unexpected HTTP status {response.status}."
            elif not isinstance(payload, dict) or payload.get("v") != 1:
                result["detail"] = "Response is not a version-1 JSON object."
            else:
                result["status"] = "pass"
    except urllib.error.HTTPError as exc:
        result["http_status"] = exc.code
        result["detail"] = f"HTTP error {exc.code}."
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as exc:
        result["detail"] = str(exc)
    result["duration_s"] = round(time.monotonic() - started, 3)
    return result


def load_checklist(path: Path | None) -> dict[str, dict[str, str]]:
    checks = {key: {"label": label, "status": "pending", "note": ""} for key, label in MANUAL_CHECKS}
    if path is None:
        return checks
    raw = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(raw, dict):
        raise ValueError("Checklist file must contain a JSON object.")
    for key, value in raw.items():
        if key not in checks:
            raise ValueError(f"Unknown checklist key: {key}")
        if isinstance(value, str):
            status, note = value, ""
        elif isinstance(value, dict):
            status = str(value.get("status", "pending"))
            note = str(value.get("note", ""))
        else:
            raise ValueError(f"Invalid checklist value for {key}")
        if status not in ALLOWED_CHECKLIST_STATUSES:
            raise ValueError(f"Invalid checklist status for {key}: {status}")
        checks[key]["status"] = status
        checks[key]["note"] = note
    return checks


def prompt_checklist(checks: dict[str, dict[str, str]]) -> None:
    print("\nManual hardware checklist. Enter pass, fail, skip or pending.")
    for key, entry in checks.items():
        while True:
            answer = input(f"[{key}] {entry['label']}\nstatus: ").strip().lower() or "pending"
            if answer in ALLOWED_CHECKLIST_STATUSES:
                break
            print("Invalid status.")
        note = input("note (optional): ").strip()
        entry["status"] = answer
        entry["note"] = note



def write_checklist_file(path: Path, checks: dict[str, dict[str, str]]) -> None:
    payload = {
        key: {"status": item["status"], "note": item["note"]}
        for key, item in checks.items()
    }
    write_text(path, json.dumps(payload, indent=2, ensure_ascii=False) + "\n")


def load_existing_report(path: Path) -> ValidationReport:
    raw = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(raw, dict):
        raise ValueError("Existing report must contain a JSON object.")
    steps_raw = raw.get("steps", [])
    if not isinstance(steps_raw, list):
        raise ValueError("Existing report steps must be a list.")
    steps = [StepResult(**item) for item in steps_raw if isinstance(item, dict)]
    return ValidationReport(
        started_at=str(raw.get("started_at", now_iso())),
        finished_at=str(raw.get("finished_at", "")),
        project_root=str(raw.get("project_root", PROJECT_ROOT)),
        arguments=dict(raw.get("arguments", {})),
        devices=list(raw.get("devices", [])),
        steps=steps,
        api_checks=list(raw.get("api_checks", [])),
        log_evidence=dict(raw.get("log_evidence", {})),
        manual_checks=dict(raw.get("manual_checks", {})),
        checklist_file=raw.get("checklist_file"),
        overall_status=str(raw.get("overall_status", "running")),
    )


def write_reports(report: ValidationReport, output_dir: Path) -> tuple[Path, Path]:
    json_path = output_dir / "report.json"
    md_path = output_dir / "report.md"
    json_path.write_text(
        json.dumps(
            {
                **asdict(report),
                "steps": [asdict(step) for step in report.steps],
            },
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )
    md_path.write_text(report_to_markdown(report), encoding="utf-8")
    return json_path, md_path

def report_to_markdown(report: ValidationReport) -> str:
    lines = [
        "# PulseMon P8 validation report",
        "",
        f"- Started: `{report.started_at}`",
        f"- Finished: `{report.finished_at}`",
        f"- Overall status: **{report.overall_status}**",
        f"- Project root: `{report.project_root}`",
        f"- Hardware checklist: `{report.checklist_file or 'not generated'}`",
        "",
        "## Automated steps",
        "",
        "| Step | Status | Return code | Duration |",
        "|---|---:|---:|---:|",
    ]
    for step in report.steps:
        rc = "" if step.returncode is None else str(step.returncode)
        lines.append(f"| `{step.name}` | {step.status} | {rc} | {step.duration_s:.3f}s |")
    if report.api_checks:
        lines.extend(["", "## Backend API checks", "", "| Endpoint | Status | HTTP | Duration | Detail |", "|---|---:|---:|---:|---|"])
        for item in report.api_checks:
            lines.append(
                f"| `{item['endpoint']}` | {item['status']} | {item['http_status'] or ''} | "
                f"{item['duration_s']:.3f}s | {item['detail']} |"
            )
    if report.devices:
        lines.extend(["", "## Detected serial devices", ""])
        for item in report.devices:
            port = item.get("port", "unknown")
            description = item.get("description", "")
            lines.append(f"- `{port}` — {description}")
    if report.log_evidence:
        lines.extend(["", "## Serial log evidence", ""])
        for key, found in report.log_evidence.items():
            lines.append(f"- `{key}`: {'observed' if found else 'not observed'}")
    lines.extend(["", "## Manual hardware checks", "", "| Check | Status | Note |", "|---|---:|---|"])
    for key, item in report.manual_checks.items():
        lines.append(f"| `{key}` — {item['label']} | {item['status']} | {item['note']} |")
    lines.extend(
        [
            "",
            "## Interpretation",
            "",
            "- `pass`: requested check completed successfully.",
            "- `partial`: automated checks passed, but one or more hardware checks remain pending or skipped.",
            "- `fail`: at least one requested automated or manual check failed.",
            "- `blocked`: a required prerequisite or device was unavailable.",
            "",
        ]
    )
    return "\n".join(lines)


def determine_overall(report: ValidationReport, *, require_hardware: bool) -> str:
    statuses = [step.status for step in report.steps]
    if any(status == "fail" for status in statuses):
        return "fail"
    if any(status == "blocked" for status in statuses):
        return "blocked"
    if any(item.get("status") == "fail" for item in report.api_checks):
        return "fail"
    manual_statuses = [item["status"] for item in report.manual_checks.values()]
    if "fail" in manual_statuses:
        return "fail"
    if require_hardware and any(status != "pass" for status in manual_statuses):
        return "blocked"
    if any(status in {"pending", "skip"} for status in manual_statuses):
        return "partial"
    return "pass"


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build and validate PulseMon P8 on a local machine.")
    parser.add_argument("--output-dir", type=Path, help="Report directory. Default: .pulsemon-results/p8/<timestamp>.")
    parser.add_argument(
        "--codex-full",
        action="store_true",
        help=(
            "Run the explicitly authorized complete P8 automated profile: backend tests, both builds, "
            "release flash, 180-second serial capture and API checks against 192.168.0.10:8000."
        ),
    )
    parser.add_argument(
        "--resume-report",
        type=Path,
        help="Finalize an existing report with a completed hardware checklist without rerunning builds or tests.",
    )
    parser.add_argument("--skip-backend-tests", action="store_true", help="Skip backend config precheck and pytest.")
    parser.add_argument("--skip-builds", action="store_true", help="Skip both PlatformIO builds.")
    parser.add_argument("--release-only", action="store_true", help="Build only the release environment.")
    parser.add_argument("--debug-only", action="store_true", help="Build only the debug environment.")
    parser.add_argument("--flash", action="store_true", help="Explicitly upload firmware after successful builds.")
    parser.add_argument("--flash-env", choices=(RELEASE_ENV, DEBUG_ENV), default=RELEASE_ENV)
    parser.add_argument("--port", help="Serial/upload port. Auto-selected only when exactly one port is detected.")
    parser.add_argument("--monitor-seconds", type=int, help="Capture serial output for N seconds; omitted means disabled.")
    parser.add_argument(
        "--backend-url",
        help=f"Backend base URL to verify. --codex-full defaults to {DEFAULT_BACKEND_URL}.",
    )
    parser.add_argument("--api-key-env", default="PULSEMON_TEST_API_KEY", help="Environment variable containing the API key.")
    parser.add_argument("--api-key-header", default="X-API-Key", help="Header used for the optional API key.")
    parser.add_argument("--api-timeout", type=float, default=5.0)
    parser.add_argument("--checklist-file", type=Path, help="JSON file containing manual check statuses.")
    parser.add_argument("--interactive-checklist", action="store_true", help="Prompt for manual hardware check results.")
    parser.add_argument("--require-hardware", action="store_true", help="Fail unless every manual hardware check passes.")
    parser.add_argument("--dry-run", action="store_true", help="Write the plan and reports without executing commands.")
    args = parser.parse_args(argv)

    if args.release_only and args.debug_only:
        parser.error("--release-only and --debug-only are mutually exclusive")
    if args.api_timeout <= 0:
        parser.error("--api-timeout must be positive")
    if args.resume_report and args.codex_full:
        parser.error("--resume-report and --codex-full are mutually exclusive")
    if args.resume_report and not args.checklist_file:
        parser.error("--resume-report requires --checklist-file")

    if args.codex_full:
        forbidden = []
        if args.skip_backend_tests:
            forbidden.append("--skip-backend-tests")
        if args.skip_builds:
            forbidden.append("--skip-builds")
        if args.release_only:
            forbidden.append("--release-only")
        if args.debug_only:
            forbidden.append("--debug-only")
        if forbidden:
            parser.error("--codex-full cannot be combined with " + ", ".join(forbidden))
        args.flash = True
        args.flash_env = RELEASE_ENV
        args.monitor_seconds = CODEX_MONITOR_SECONDS if args.monitor_seconds is None else args.monitor_seconds
        args.backend_url = args.backend_url or DEFAULT_BACKEND_URL

    if args.monitor_seconds is None:
        args.monitor_seconds = 0
    if args.monitor_seconds < 0:
        parser.error("--monitor-seconds must be non-negative")
    if args.codex_full and args.monitor_seconds <= 0:
        parser.error("--codex-full requires a positive --monitor-seconds value")
    return args


def print_report_paths(report: ValidationReport, json_path: Path, md_path: Path) -> None:
    print(f"P8 status: {report.overall_status}")
    print(f"JSON report: {json_path}")
    print(f"Markdown report: {md_path}")
    if report.checklist_file:
        print(f"Hardware checklist: {report.checklist_file}")


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)

    if args.resume_report:
        try:
            report = load_existing_report(args.resume_report.resolve())
            report.manual_checks = load_checklist(args.checklist_file.resolve())
        except (OSError, TypeError, ValueError, json.JSONDecodeError) as exc:
            print(f"Resume error: {exc}", file=sys.stderr)
            return 2
        output_dir = (args.output_dir or args.resume_report.resolve().parent).resolve()
        output_dir.mkdir(parents=True, exist_ok=True)
        report.arguments = {
            **report.arguments,
            "finalization": sanitize_args(args),
        }
        report.checklist_file = str(args.checklist_file.resolve())
        report.finished_at = now_iso()
        report.overall_status = determine_overall(report, require_hardware=args.require_hardware)
        json_path, md_path = write_reports(report, output_dir)
        print_report_paths(report, json_path, md_path)
        return 1 if report.overall_status in {"fail", "blocked"} else 0

    timestamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    output_dir = args.output_dir or (PROJECT_ROOT / ".pulsemon-results" / "p8" / timestamp)
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    report = ValidationReport(started_at=now_iso(), arguments=sanitize_args(args))
    try:
        report.manual_checks = load_checklist(args.checklist_file)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"Checklist error: {exc}", file=sys.stderr)
        return 2

    python = choose_python()
    if not args.skip_backend_tests:
        report.steps.append(
            run_command(
                name="backend-config-precheck",
                command=[python, "-m", "app.config"],
                cwd=API_DIR,
                output_dir=output_dir,
                timeout=60,
                dry_run=args.dry_run,
            )
        )
        report.steps.append(
            run_command(
                name="backend-tests",
                command=[python, "-m", "pytest", "-q", "-p", "no:cacheprovider"],
                cwd=API_DIR,
                output_dir=output_dir,
                timeout=600,
                dry_run=args.dry_run,
            )
        )

    needs_pio = not args.skip_builds or args.flash or args.monitor_seconds > 0
    if needs_pio and not args.dry_run and not command_exists("pio"):
        report.steps.append(StepResult(name="platformio-prerequisite", status="blocked", detail="pio not found in PATH."))
    else:
        build_envs: list[str] = []
        if not args.skip_builds:
            if args.debug_only:
                build_envs = [DEBUG_ENV]
            elif args.release_only:
                build_envs = [RELEASE_ENV]
            else:
                build_envs = [RELEASE_ENV, DEBUG_ENV]
            for environment in build_envs:
                report.steps.append(
                    run_command(
                        name=f"build-{environment}",
                        command=["pio", "run", "-e", environment],
                        cwd=ESP_DIR,
                        output_dir=output_dir,
                        timeout=1800,
                        dry_run=args.dry_run,
                    )
                )

        if args.flash or args.monitor_seconds > 0:
            devices, detection = detect_devices(dry_run=args.dry_run)
            report.devices = devices
            report.steps.append(detection)
            port = resolve_port(args.port, devices)
            if not port and not args.dry_run:
                report.steps.append(
                    StepResult(
                        name="serial-port-selection",
                        status="blocked",
                        detail="Specify --port or expose exactly one serial device.",
                    )
                )
            else:
                selected_port = port or args.port or "DRY-RUN-PORT"
                flash_step: StepResult | None = None
                if args.flash:
                    required_build = next(
                        (step for step in report.steps if step.name == f"build-{args.flash_env}"),
                        None,
                    )
                    if required_build is not None and required_build.status not in {"pass", "dry-run"}:
                        flash_step = StepResult(
                            name=f"flash-{args.flash_env}",
                            status="blocked",
                            detail=f"Flash skipped because build-{args.flash_env} did not pass.",
                        )
                    else:
                        flash_step = run_command(
                            name=f"flash-{args.flash_env}",
                            command=["pio", "run", "-e", args.flash_env, "-t", "upload", "--upload-port", selected_port],
                            cwd=ESP_DIR,
                            output_dir=output_dir,
                            timeout=600,
                            dry_run=args.dry_run,
                        )
                    report.steps.append(flash_step)
                if args.monitor_seconds > 0:
                    if flash_step is not None and flash_step.status not in {"pass", "dry-run"}:
                        report.steps.append(
                            StepResult(
                                name="serial-monitor",
                                status="blocked",
                                detail="Serial monitor skipped because firmware flashing did not pass.",
                            )
                        )
                    else:
                        monitor = capture_monitor(
                            port=selected_port,
                            seconds=args.monitor_seconds,
                            output_dir=output_dir,
                            dry_run=args.dry_run,
                        )
                        report.steps.append(monitor)
                        report.log_evidence = inspect_monitor_log(monitor)

    if args.backend_url:
        api_key = os.environ.get(args.api_key_env)
        for endpoint in DEFAULT_ENDPOINTS:
            if args.dry_run:
                report.api_checks.append(
                    {
                        "endpoint": endpoint,
                        "url": args.backend_url.rstrip("/") + endpoint,
                        "status": "dry-run",
                        "http_status": None,
                        "duration_s": 0.0,
                        "detail": "Request not executed.",
                    }
                )
            else:
                report.api_checks.append(
                    api_check(
                        base_url=args.backend_url,
                        endpoint=endpoint,
                        timeout=args.api_timeout,
                        header_name=args.api_key_header,
                        api_key=api_key,
                    )
                )

    if args.interactive_checklist:
        if not sys.stdin.isatty():
            report.steps.append(
                StepResult(
                    name="interactive-checklist",
                    status="blocked",
                    detail="Interactive checklist requested without a TTY.",
                )
            )
        else:
            prompt_checklist(report.manual_checks)

    checklist_path = (args.checklist_file or (output_dir / "hardware-checklist.json")).resolve()
    write_checklist_file(checklist_path, report.manual_checks)
    report.checklist_file = str(checklist_path)
    report.finished_at = now_iso()
    report.overall_status = determine_overall(report, require_hardware=args.require_hardware)
    json_path, md_path = write_reports(report, output_dir)
    print_report_paths(report, json_path, md_path)

    if report.overall_status in {"fail", "blocked"}:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
