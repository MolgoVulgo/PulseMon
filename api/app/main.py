from contextlib import asynccontextmanager
import logging
import threading
import time

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse, JSONResponse, RedirectResponse

from app.config import AppConfig, load_config
from app.diagnostics.logging import configure_logging
from app.diagnostics.probe import capture_gpu_raw_vs_display, capture_raw_metrics, probe_sources
from app.models import (
    DashboardResponse,
    DbFanDeleteResponse,
    DbFansListResponse,
    DbFanWriteRequest,
    DbFanWriteResponse,
    DbDataResponse,
    ErrorResponse,
    FansConfigResponse,
    FansConfigUpdateRequest,
    FansDashboardResponse,
    FansMetaResponse,
    FansReferenceResponse,
    GpuDashboardResponse,
    GpuHistoryResponse,
    GpuMetaResponse,
    HealthResponse,
    HistoryResponse,
    MetaResponse,
    UserConfigResponse,
    UserConfigUpdateRequest,
)
from app.services import (
    GpuSnapshotUnavailableError,
    SamplerService,
    SnapshotUnavailableError,
    build_dashboard_from_store,
    build_dashboard_live,
    build_fans_dashboard,
    build_fans_meta,
    get_fans_reference_catalog,
    get_fans_mapping_config,
    build_gpu_dashboard_from_store,
    build_gpu_dashboard_live,
    build_gpu_history,
    build_gpu_meta,
    build_history,
    build_meta,
    save_fans_mapping_config,
    get_user_config,
    save_user_config,
    get_db_data_view,
    create_db_fan,
    hard_delete_db_fan,
    list_db_fans,
    restore_db_fan,
    soft_delete_db_fan,
    update_db_fan,
)
from app.store import GpuHistoryStore, GpuSnapshotStore, HistoryStore, SnapshotStore
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
gpu_snapshot_store = GpuSnapshotStore()
gpu_history_store = GpuHistoryStore(capacity=config.history_capacity)
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
def get_history(
    window: int = WINDOW_DEFAULT,
    step: int = STEP_DEFAULT,
    mode: str = "display",
    since_ts_ms: int | None = None,
) -> HistoryResponse:
    if window < WINDOW_MIN or window > WINDOW_MAX:
        _raise_api_error(400, "invalid_parameter", "window")
    if step < STEP_MIN or step > STEP_MAX:
        _raise_api_error(400, "invalid_parameter", "step")
    if mode not in {"display", "raw"}:
        _raise_api_error(400, "invalid_parameter", "mode")
    if since_ts_ms is not None and since_ts_ms < 0:
        _raise_api_error(400, "invalid_parameter", "since_ts_ms")

    return build_history(
        history_store=history_store,
        window_s=window,
        step_s=step,
        mode=mode,
        since_ts_ms=since_ts_ms,
    )


@app.get("/api/v1/meta", response_model=MetaResponse)
def get_meta() -> MetaResponse:
    return build_meta()


@app.get("/api/v1/gpu/dashboard", response_model=GpuDashboardResponse)
def get_gpu_dashboard() -> GpuDashboardResponse:
    snapshot = build_gpu_dashboard_live()
    now_ms = int(time.time() * 1000)
    gpu_snapshot_store.set_snapshot(snapshot, tick_ms=now_ms)
    gpu_history_store.push_snapshot(snapshot, tick_ms=now_ms)
    try:
        return build_gpu_dashboard_from_store(gpu_snapshot_store)
    except GpuSnapshotUnavailableError:
        _raise_api_error(503, "snapshot_unavailable")


@app.get("/api/v1/gpu/history", response_model=GpuHistoryResponse)
def get_gpu_history(window: int = WINDOW_DEFAULT, step: int = STEP_DEFAULT, mode: str = "display") -> GpuHistoryResponse:
    if window < WINDOW_MIN or window > WINDOW_MAX:
        _raise_api_error(400, "invalid_parameter", "window")
    if step < STEP_MIN or step > STEP_MAX:
        _raise_api_error(400, "invalid_parameter", "step")
    if mode not in {"display", "raw"}:
        _raise_api_error(400, "invalid_parameter", "mode")
    if len(gpu_history_store) == 0:
        warmup = build_gpu_dashboard_live()
        now_ms = int(time.time() * 1000)
        gpu_snapshot_store.set_snapshot(warmup, tick_ms=now_ms)
        gpu_history_store.push_snapshot(warmup, tick_ms=now_ms)
    return build_gpu_history(history_store=gpu_history_store, window_s=window, step_s=step, mode=mode)


@app.get("/api/v1/gpu/meta", response_model=GpuMetaResponse)
def get_gpu_meta() -> GpuMetaResponse:
    return build_gpu_meta()


@app.get("/api/v1/fans/dashboard", response_model=FansDashboardResponse)
def get_fans_dashboard() -> FansDashboardResponse:
    return build_fans_dashboard()


@app.get("/api/v1/fans/meta", response_model=FansMetaResponse)
def get_fans_meta() -> FansMetaResponse:
    return build_fans_meta()


@app.get("/api/v1/fans/config", response_model=FansConfigResponse)
def get_fans_config() -> FansConfigResponse:
    payload = get_fans_mapping_config()
    return FansConfigResponse(
        v=1,
        mapping_path=payload.get("mapping_path", ""),
        allowed_roles=payload.get("allowed_roles", ["unknown"]),
        mappings=payload.get("mappings", []),
    )


@app.put("/api/v1/fans/config", response_model=FansConfigResponse)
def put_fans_config(request: FansConfigUpdateRequest) -> FansConfigResponse:
    payload = save_fans_mapping_config({"mappings": [item.model_dump() for item in request.mappings]})
    return FansConfigResponse(
        v=1,
        mapping_path=payload.get("mapping_path", ""),
        allowed_roles=payload.get("allowed_roles", ["unknown"]),
        mappings=payload.get("mappings", []),
    )


@app.get("/api/v1/fans/reference", response_model=FansReferenceResponse)
def get_fans_reference() -> FansReferenceResponse:
    payload = get_fans_reference_catalog()
    items = payload.get("items", [])
    return FansReferenceResponse(
        v=1,
        generated_at=payload.get("generated_at"),
        count=len(items) if isinstance(items, list) else 0,
        items=items if isinstance(items, list) else [],
    )


@app.get("/api/v1/user/config", response_model=UserConfigResponse)
def get_user_config_v1() -> UserConfigResponse:
    payload = get_user_config()
    return UserConfigResponse(v=1, settings=payload.get("settings", {}))


@app.put("/api/v1/user/config", response_model=UserConfigResponse)
def put_user_config_v1(request: UserConfigUpdateRequest) -> UserConfigResponse:
    payload = save_user_config({"settings": request.settings})
    return UserConfigResponse(v=1, settings=payload.get("settings", {}))


@app.get("/api/v1/db/data", response_model=DbDataResponse)
def get_db_data() -> DbDataResponse:
    payload = get_db_data_view()
    return DbDataResponse(
        v=1,
        db_path=str(payload.get("db_path", "")),
        fan_mappings=payload.get("fan_mappings", []),
        user_settings=payload.get("user_settings", {}),
    )


@app.get("/api/v1/db/fans", response_model=DbFansListResponse)
def get_db_fans(include_deleted: bool = True) -> DbFansListResponse:
    payload = list_db_fans(include_deleted=include_deleted)
    return DbFansListResponse(v=1, items=payload.get("items", []))


@app.post("/api/v1/db/fans", response_model=DbFanWriteResponse)
def post_db_fan(request: DbFanWriteRequest) -> DbFanWriteResponse:
    payload = create_db_fan(request.mapping.model_dump())
    return DbFanWriteResponse(v=1, item=payload)


@app.put("/api/v1/db/fans/{fan_id}", response_model=DbFanWriteResponse)
def put_db_fan(fan_id: int, request: DbFanWriteRequest) -> DbFanWriteResponse:
    try:
        payload = update_db_fan(fan_id, request.mapping.model_dump())
    except KeyError:
        _raise_api_error(404, "not_found", "fan_id")
    return DbFanWriteResponse(v=1, item=payload)


@app.post("/api/v1/db/fans/{fan_id}/soft-delete", response_model=DbFanWriteResponse)
def post_db_fan_soft_delete(fan_id: int) -> DbFanWriteResponse:
    try:
        payload = soft_delete_db_fan(fan_id)
    except KeyError:
        _raise_api_error(404, "not_found", "fan_id")
    return DbFanWriteResponse(v=1, item=payload)


@app.post("/api/v1/db/fans/{fan_id}/restore", response_model=DbFanWriteResponse)
def post_db_fan_restore(fan_id: int) -> DbFanWriteResponse:
    try:
        payload = restore_db_fan(fan_id)
    except KeyError:
        _raise_api_error(404, "not_found", "fan_id")
    return DbFanWriteResponse(v=1, item=payload)


@app.delete("/api/v1/db/fans/{fan_id}", response_model=DbFanDeleteResponse)
def delete_db_fan(fan_id: int) -> DbFanDeleteResponse:
    try:
        payload = hard_delete_db_fan(fan_id)
    except KeyError:
        _raise_api_error(404, "not_found", "fan_id")
    return DbFanDeleteResponse(v=1, deleted_id=int(payload.get("deleted_id", fan_id)))


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
