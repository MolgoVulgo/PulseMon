from contextlib import asynccontextmanager
import logging
import time

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse, JSONResponse, RedirectResponse

from app.config import AppConfig, load_config
from app.diagnostics.logging import configure_logging
from app.diagnostics.probe import probe_sources
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

WINDOW_MIN = 10
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
    sample_func=build_dashboard_live,
    interval_s=config.sample_interval_s,
)


@asynccontextmanager
async def lifespan(_: FastAPI):
    logger.info(
        "service starting bind=%s:%s sample_interval_s=%s history_capacity=%s api_key_enabled=%s",
        config.bind_host,
        config.bind_port,
        config.sample_interval_s,
        config.history_capacity,
        config.api_key is not None,
    )
    if config.diagnostics_enabled:
        logger.info("sensor probe=%s", probe_sources())

    sampler_service.start()
    try:
        yield
    finally:
        sampler_service.stop()
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
