from __future__ import annotations

from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
ESP_SRC = PROJECT_ROOT / "esp" / "src"


def _read(relative_path: str) -> str:
    return (PROJECT_ROOT / relative_path).read_text(encoding="utf-8")


def test_backend_endpoint_has_nvs_settings_module_and_compiled_fallback() -> None:
    header = _read("esp/src/pulsemon_api_settings.h")
    source = _read("esp/src/pulsemon_api_settings.c")
    config = _read("esp/src/pulsemon_api_config.h")

    assert "pulsemon_api_settings_t" in header
    assert 'NVS_NAMESPACE = "pulsemon_api"' in source
    assert 'NVS_KEY_HOST = "host"' in source
    assert 'NVS_KEY_PORT = "port"' in source
    assert "nvs_get_str" in source
    assert "nvs_get_u16" in source
    assert "nvs_set_str" in source
    assert "nvs_set_u16" in source
    assert "PULSEMON_API_DEFAULT_HOST" in config
    assert "PULSEMON_API_DEFAULT_PORT" in config
    assert "PULSEMON_API_BASE_URL" not in config


def test_api_client_builds_urls_from_cached_runtime_endpoint() -> None:
    client = _read("esp/src/pulsemon_api_client.c")

    assert "pulsemon_api_client_reload_endpoint" in client
    assert "pulsemon_api_client_get_endpoint" in client
    assert "pulsemon_api_settings_build_base_url" in client
    assert 'build_endpoint_url("/dashboard"' in client
    assert 'build_endpoint_url("/gpu/dashboard"' in client
    assert "PULSEMON_API_BASE_URL" not in client
    assert client.count("pulsemon_api_settings_load(&settings)") == 1


def test_local_portal_exposes_and_applies_backend_host_and_port() -> None:
    portal = _read("esp/src/wifi_config_server.c")

    assert 'id=\\"backend_host\\"' in portal
    assert 'id=\\"backend_port\\"' in portal
    assert 'form_get(body, "backend_host"' in portal
    assert 'form_get_i32(body, "backend_port"' in portal
    assert r'\"backend_host\":' in portal
    assert r'\"backend_port\":' in portal
    assert "pulsemon_api_settings_save(&api_settings)" in portal
    assert "pulsemon_api_client_reload_endpoint()" in portal
    assert "pulsemon_api_settings_clear()" in portal


def test_endpoint_validation_rejects_url_components_and_invalid_ports() -> None:
    settings = _read("esp/src/pulsemon_api_settings.c")
    portal = _read("esp/src/wifi_config_server.c")

    assert "isalnum(c)" in settings
    assert "c == '-'" in settings
    assert "c == '.'" in settings
    assert "return false;" in settings
    assert "backend_port < 1 || backend_port > UINT16_MAX" in portal
    assert 'send_text(req, 400, "invalid backend endpoint")' in portal


def test_documentation_describes_nvs_endpoint_and_compiled_fallback() -> None:
    for relative_path in (
        "README.md",
        "README.fr.md",
        "docs/architecture.md",
        "docs/fr/architecture.md",
        "docs/configuration.md",
        "docs/fr/configuration.md",
        "docs/firmware.md",
        "docs/fr/firmware.md",
        "docs/web-configuration.md",
        "docs/fr/web-configuration.md",
    ):
        text = _read(relative_path)
        assert "pulsemon_api" in text, relative_path

    config_en = _read("docs/configuration.md")
    config_fr = _read("docs/fr/configuration.md")
    for name in (
        "PULSEMON_API_DEFAULT_HOST",
        "PULSEMON_API_DEFAULT_PORT",
        "PULSEMON_HTTP_TIMEOUT_MS",
        "PULSEMON_DASHBOARD_POLL_MS",
    ):
        assert name in config_en
        assert name in config_fr
