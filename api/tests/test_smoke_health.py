from app.main import app


def test_health_route_registered() -> None:
    routes = {
        route.path
        for route in app.routes
        if getattr(route, "methods", None) and "GET" in route.methods
    }

    assert "/api/v1/health" in routes
