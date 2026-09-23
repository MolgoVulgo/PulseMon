from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def _read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def test_portal_lifecycle_follows_real_ap_events() -> None:
    manager = _read("esp/src/wifi_manager.c")
    header = _read("esp/src/wifi_manager.h")
    main = _read("esp/src/main.c")

    assert "pulsemon_wifi_config_mode_cb_t" in header
    assert "WIFI_EVENT_AP_START" in manager
    assert "WIFI_EVENT_AP_STOP" in manager
    assert "set_ap_active(true);" in manager
    assert "set_ap_active(false);" in manager
    assert "s_config_mode_cb(active);" in manager

    assert "pulsemon_on_config_mode_changed" in main
    assert "pulsemon_wifi_config_server_start()" in main
    assert "pulsemon_wifi_captive_dns_start()" in main
    assert "pulsemon_wifi_captive_dns_stop()" in main
    assert "pulsemon_wifi_config_server_stop()" in main
    assert main.index("pulsemon_wifi_captive_dns_stop()") < main.index("pulsemon_wifi_config_server_stop()")


def test_http_and_dns_are_not_started_unconditionally_during_boot() -> None:
    main = _read("esp/src/main.c")
    app_main = main.split("void app_main(void)", 1)[1]

    assert "pulsemon_wifi_config_server_start()" not in app_main
    assert "pulsemon_wifi_captive_dns_start()" not in app_main


def test_config_services_expose_explicit_stop_contracts() -> None:
    server_header = _read("esp/src/wifi_config_server.h")
    server = _read("esp/src/wifi_config_server.c")
    dns_header = _read("esp/src/wifi_captive_dns.h")
    dns = _read("esp/src/wifi_captive_dns.c")

    assert "pulsemon_wifi_config_server_stop" in server_header
    assert "httpd_stop(server)" in server
    assert "s_server = NULL" in server
    assert "portal_mode_required" in server
    assert "configuration portal inactive" in server
    assert server.count("if (!portal_mode_required(req))") == 9

    assert "pulsemon_wifi_captive_dns_stop" in dns_header
    assert "s_stop_requested = true" in dns
    assert "shutdown(sock, SHUT_RDWR)" in dns
    assert "while (!s_stop_requested)" in dns


def test_captive_dns_binds_only_to_the_setup_ap_address() -> None:
    config = _read("esp/src/wifi_config.h")
    dns = _read("esp/src/wifi_captive_dns.c")

    assert '#define PULSEMON_WIFI_AP_IPV4 "192.168.4.1"' in config
    assert "inet_addr(PULSEMON_WIFI_AP_IPV4)" in dns
    assert "INADDR_ANY" not in dns


def test_portal_reports_manual_window_and_stops_presenting_stale_form() -> None:
    server = _read("esp/src/wifi_config_server.c")

    assert 'manual_config_active' in server
    assert 'manual_config_remaining_ms' in server
    assert "AbortController" in server
    assert "cache:'no-store'" in server
    assert "setTimeout(pollPortal,2000)" in server
    assert "setTimeout(status,2000)" in server
    assert "Configuration window closed. Reopen PulseMon-Setup from the display." in server
    assert "document.querySelectorAll('button,input,select,a')" in server

def test_http_server_has_capacity_for_every_registered_portal_route() -> None:
    server = _read("esp/src/wifi_config_server.c")

    assert "config.max_uri_handlers = 10;" in server
    assert "const httpd_uri_t *uris[]" in server
    assert "httpd_register_uri_handler(s_server, uris[i])" in server
    assert "unable to register uri %s" in server
    assert "httpd_stop(s_server);" in server
    assert "s_server = NULL;" in server

