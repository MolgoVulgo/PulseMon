from __future__ import annotations

from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def _read(relative_path: str) -> str:
    return (PROJECT_ROOT / relative_path).read_text(encoding="utf-8")


def test_printer_credentials_live_only_in_nvs_settings() -> None:
    header = _read("esp/src/printer_settings.h")
    source = _read("esp/src/printer_settings.c")
    config = _read("esp/src/printer_config.h")

    assert "printer_settings_t" in header
    assert 'NVS_NAMESPACE = "printer"' in source
    assert 'NVS_KEY_HOST = "host"' in source
    assert 'NVS_KEY_ACCESS_CODE = "access_code"' in source
    assert "nvs_get_str" in source
    assert "nvs_set_str" in source
    assert "nvs_erase_all" in source

    assert "PRINTER_HOST" not in config
    assert "PRINTER_ACCESS_CODE" not in config
    assert "PRINTER_HTTP_PORT" in config
    assert "PRINTER_MQTT_PORT" in config


def test_printer_service_uses_runtime_settings_and_can_reload() -> None:
    service = _read("esp/src/printer_service.c")
    header = _read("esp/src/printer_service.h")

    assert '#include "printer_settings.h"' in service
    assert "printer_settings_load(&settings)" in service
    assert "s_settings.host" in service
    assert "s_settings.access_code" in service
    assert "PRINTER_HOST" not in service
    assert "PRINTER_ACCESS_CODE" not in service
    assert "printer_service_reload_settings" in header
    assert "state_request_settings_reload();" in service
    assert "mqtt_destroy();" in service


def test_local_portal_configures_printer_without_returning_secret() -> None:
    portal = _read("esp/src/wifi_config_server.c")

    assert 'id=\\"printer_host\\"' in portal
    assert 'id=\\"printer_access_code\\"' in portal
    assert 'id=\\"clear_printer_config\\"' in portal
    assert 'form_get(body, "printer_host"' in portal
    assert 'form_get(body, "printer_access_code"' in portal
    assert 'form_get(body, "clear_printer_config"' in portal
    assert "printer_settings_save(&printer_settings)" in portal
    assert "printer_settings_clear()" in portal
    assert "printer_service_reload_settings();" in portal
    assert r'\"printer_host\":\"%s\"' in portal
    assert r'\"printer_access_code_set\":%s' in portal
    assert r'\"printer_access_code\":' not in portal


def test_documentation_describes_persistent_printer_configuration() -> None:
    for relative_path in (
        "README.md",
        "README.fr.md",
        "docs/configuration.md",
        "docs/fr/configuration.md",
        "docs/firmware.md",
        "docs/fr/firmware.md",
        "docs/web-configuration.md",
        "docs/fr/web-configuration.md",
    ):
        text = _read(relative_path)
        assert "printer" in text.lower(), relative_path

    config_en = _read("docs/configuration.md")
    config_fr = _read("docs/fr/configuration.md")
    assert "namespace: printer" in config_en
    assert "keys: host, access_code" in config_en
    assert "namespace : printer" in config_fr
    assert "clés : host, access_code" in config_fr


def test_printer_active_job_cache_survives_screen_exit_in_psram() -> None:
    service = _read("esp/src/printer_service.c")
    service_header = _read("esp/src/printer_service.h")
    screen = _read("esp/src/ui_screen.c")
    thumbnail_fetch = _read("esp/src/printer_thumbnail_fetch.c")

    assert "printer_display_cache_t" in service
    assert "MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT" in service
    assert "printer display cache allocated in PSRAM" in service
    assert "printer_service_has_cached_display" in service_header
    assert "printer_service_restore_cached_display" in service_header
    assert "display_cache_update_status" in service
    assert "display_cache_update_name" in service

    mqtt_destroy = service.split("static void mqtt_destroy(void)", 1)[1].split("static bool mqtt_start", 1)[0]
    assert "state_reset_job" not in mqtt_destroy
    assert "printer_thumbnail_clear" not in mqtt_destroy

    stop_body = service.split("void printer_service_stop(void)", 1)[1].split("void printer_service_reload_settings", 1)[0]
    assert "display_cache_clear" not in stop_body
    assert "printer_thumbnail_clear" not in stop_body

    assert "printer_service_restore_cached_display();" in screen
    assert "printer_service_has_cached_display();" in screen
    assert "bool displayable = available || cached;" in screen

    assert "heap_caps_malloc(decoded_cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)" in thumbnail_fetch
    assert "decoded = malloc(decoded_cap)" not in thumbnail_fetch


def test_printer_settings_reload_invalidates_cached_job() -> None:
    service = _read("esp/src/printer_service.c")
    reload_body = service.split("void printer_service_reload_settings(void)", 1)[1].split("void printer_service_request_update", 1)[0]

    assert "display_cache_clear();" in reload_body
    assert "state_reset_job();" in reload_body
    assert "printer_thumbnail_clear();" in reload_body
