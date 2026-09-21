from app.main import app


REMOVED_ROUTES = {
    "/api/v1/fans/dashboard",
    "/api/v1/fans/meta",
    "/api/v1/fans/config",
    "/api/v1/fans/reference",
    "/api/v1/user/config",
    "/api/v1/db/data",
    "/api/v1/db/fans",
    "/api/v1/db/fans/{fan_id}",
    "/api/v1/db/fans/{fan_id}/soft-delete",
    "/api/v1/db/fans/{fan_id}/restore",
}


def test_removed_fan_and_sqlite_routes_are_not_registered() -> None:
    registered = {route.path for route in app.routes}
    assert REMOVED_ROUTES.isdisjoint(registered)


def test_active_gpu_fan_telemetry_route_is_preserved() -> None:
    registered = {route.path for route in app.routes}
    assert "/api/v1/gpu/dashboard" in registered
    assert "/api/v1/gpu/history" in registered
    assert "/api/v1/gpu/meta" in registered
