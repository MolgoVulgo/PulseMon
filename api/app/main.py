from contextlib import asynccontextmanager
import logging
import threading
import time

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse, JSONResponse, RedirectResponse

from app.config import AppConfig, load_config
from app.diagnostics.logging import configure_logging
from app.diagnostics.probe import capture_gpu_raw_vs_display, capture_raw_metrics, probe_sources
from app.models import DashboardResponse, ErrorResponse, HealthResponse, HistoryResponse, MetaResponse
from app.services import (
    SamplerService,
    SnapshotUnavailableError,
    build_dashboard_from_store,
    build_dashboard_live,
    build_history,
    build_meta,
)
from app.store import HistoryStore, SnapshotStore
from app.ui import get_ui_html

WINDOW_MIN = 1
WINDOW_MAX = 600
WINDOW_DEFAULT = 300
STEP_MIN = 1
STEP_MAX = 10
STEP_DEFAULT = 1

config: AppConfig = load_config()
configure_logging(config.log_level)
logger = logging.getLogger(__name__)

snapshot_store = SnapshotStore()
history_store = HistoryStore(capacity=config.history_capacity)
sampler_service = SamplerService(
    snapshot_store=snapshot_store,
    history_store=history_store,
    sample_func=lambda: build_dashboard_live(display_ema_alpha=config.display_ema_alpha),
    interval_s=config.sample_interval_s,
    publish_interval_s=config.publish_interval_s,
)


@asynccontextmanager
async def lifespan(_: FastAPI):
    diagnostics_threads: list[threading.Thread] = []
    logger.info(
        "service starting bind=%s:%s acquire_interval_s=%s publish_interval_s=%s ema_alpha=%s history_capacity=%s api_key_enabled=%s",
        config.bind_host,
        config.bind_port,
        config.sample_interval_s,
        config.publish_interval_s,
        config.display_ema_alpha,
        config.history_capacity,
        config.api_key is not None,
    )
    if config.diagnostics_enabled:
        logger.info("sensor probe=%s", probe_sources())
        if config.diagnostics_raw_capture:
            logger.info(
                "diagnostics raw capture enabled hz=%s duration_s=%s output=%s",
                config.diagnostics_raw_hz,
                config.diagnostics_raw_duration_s,
                config.diagnostics_raw_log_path,
            )

            def _run_capture() -> None:
                try:
                    summary = capture_raw_metrics(
                        duration_s=config.diagnostics_raw_duration_s,
                        sample_hz=config.diagnostics_raw_hz,
                        output_path=config.diagnostics_raw_log_path,
                    )
                    logger.info("diagnostics raw capture finished summary=%s", summary)
                except Exception as exc:  # pragma: no cover - defensive path
                    logger.warning("diagnostics raw capture failed: %s:%s", type(exc).__name__, exc)

            diagnostics_threads.append(
                threading.Thread(
                    target=_run_capture,
                    name="stats-diag-raw-capture",
                    daemon=True,
                )
            )
            diagnostics_threads[-1].start()

        if config.diagnostics_compare_capture:
            logger.info(
                "diagnostics compare capture enabled hz=%s duration_s=%s output=%s",
                config.diagnostics_compare_hz,
                config.diagnostics_compare_duration_s,
                config.diagnostics_compare_log_path,
            )

            def _run_compare_capture() -> None:
                try:
                    summary = capture_gpu_raw_vs_display(
                        duration_s=config.diagnostics_compare_duration_s,
                        sample_hz=config.diagnostics_compare_hz,
                        alpha=config.display_ema_alpha,
                        output_path=config.diagnostics_compare_log_path,
                    )
                    logger.info("diagnostics compare capture finished summary=%s", summary)
                except Exception as exc:  # pragma: no cover - defensive path
                    logger.warning("diagnostics compare capture failed: %s:%s", type(exc).__name__, exc)

            diagnostics_threads.append(
                threading.Thread(
                    target=_run_compare_capture,
                    name="stats-diag-compare-capture",
                    daemon=True,
                )
            )
            diagnostics_threads[-1].start()

    sampler_service.start()
    try:
        yield
    finally:
        sampler_service.stop()
        for diagnostics_thread in diagnostics_threads:
            diagnostics_thread.join(timeout=0.1)
        logger.info("service stopped")


app = FastAPI(title="stats-linux-api", lifespan=lifespan)


@app.middleware("http")
async def api_key_middleware(request: Request, call_next):
    if _is_api_key_required() and request.url.path.startswith("/api/v1/"):
        given = request.headers.get(config.api_key_header)
        if given != config.api_key:
            payload = ErrorResponse(v=1, error="unauthorized", field=config.api_key_header.lower()).model_dump()
            return JSONResponse(status_code=401, content=payload)

    return await call_next(request)


@app.get("/", include_in_schema=False)
def get_root() -> RedirectResponse:
    return RedirectResponse(url="/ui")


@app.get("/ui", include_in_schema=False)
def get_ui() -> HTMLResponse:
    return HTMLResponse(content=get_ui_html())


@app.get("/api/v1/health", response_model=HealthResponse)
def get_health() -> HealthResponse:
    return HealthResponse(
        v=1,
        ts=int(time.time()),
        ok=True,
        service="stats-linux-api",
    )


@app.get("/api/v1/dashboard", response_model=DashboardResponse)
def get_dashboard() -> DashboardResponse:
    try:
        return build_dashboard_from_store(snapshot_store)
    except SnapshotUnavailableError:
        _raise_api_error(503, "snapshot_unavailable")


@app.get("/api/v1/history", response_model=HistoryResponse)
def get_history(window: int = WINDOW_DEFAULT, step: int = STEP_DEFAULT) -> HistoryResponse:
    if window < WINDOW_MIN or window > WINDOW_MAX:
        _raise_api_error(400, "invalid_parameter", "window")
    if step < STEP_MIN or step > STEP_MAX:
        _raise_api_error(400, "invalid_parameter", "step")

    return build_history(
        history_store=history_store,
        window_s=window,
        step_s=step,
    )


@app.get("/api/v1/meta", response_model=MetaResponse)
def get_meta() -> MetaResponse:
    return build_meta()


def _raise_api_error(status_code: int, error: str, field: str | None = None) -> None:
    payload = ErrorResponse(v=1, error=error, field=field).model_dump()
    raise HTTPException(status_code=status_code, detail=payload)


@app.exception_handler(HTTPException)
async def http_exception_handler(_: Request, exc: HTTPException):
    if isinstance(exc.detail, dict) and "v" in exc.detail and "error" in exc.detail:
        return JSONResponse(status_code=exc.status_code, content=exc.detail)

    payload = ErrorResponse(v=1, error="http_error", field=None).model_dump()
    return JSONResponse(status_code=exc.status_code, content=payload)


def _is_api_key_required() -> bool:
    return config.api_key is not None
