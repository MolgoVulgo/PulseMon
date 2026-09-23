from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def _read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def test_touch_hold_trigger_is_runtime_only_and_available_on_active_screens() -> None:
    trigger = _read("esp/src/config_mode_trigger.c")
    config = _read("esp/src/wifi_config.h")

    assert "PULSEMON_WIFI_LOCAL_TRIGGER_HOLD_MS 5000U" in config
    assert "PULSEMON_WIFI_LOCAL_TRIGGER_HOTSPOT_PX 64" in config
    assert "LV_EVENT_PRESSED" in trigger
    assert "LV_EVENT_PRESSING" in trigger
    assert "lv_tick_elaps(s_hold_started_ms)" in trigger
    assert "lv_obj_remove_style_all(hotspot)" in trigger
    assert "objects.main" in trigger
    assert "objects.gpu" in trigger
    assert "objects.meteo" in trigger
    assert "pulsemon_wifi_manager_open_config_mode()" in trigger


def test_trigger_is_started_only_after_wifi_manager_start_succeeds() -> None:
    main = _read("esp/src/main.c")

    manager_start = main.index("wifi_ret = pulsemon_wifi_manager_start();")
    trigger_start = main.index("pulsemon_config_mode_trigger_start();")
    assert manager_start < trigger_start
    assert "if (wifi_ret != ESP_OK)" in main[manager_start:trigger_start]
    assert '#include "config_mode_trigger.h"' in main


def test_manual_configuration_window_is_bounded_and_keeps_sta_connected() -> None:
    manager = _read("esp/src/wifi_manager.c")
    header = _read("esp/src/wifi_manager.h")
    config = _read("esp/src/wifi_config.h")

    assert "pulsemon_wifi_manager_open_config_mode" in header
    assert "PULSEMON_WIFI_MANUAL_CONFIG_TIMEOUT_MS 300000U" in config
    assert "xTimerCreate(" in manager
    assert "xTimerReset(s_manual_config_timer, 0)" in manager
    assert "s_manual_config_active" in manager
    assert "s_manual_config_deadline_us" in manager
    assert "esp_timer_get_time()" in manager
    assert "manual_config_active" in header
    assert "manual_config_remaining_ms" in header
    assert "mode != WIFI_MODE_APSTA" in manager
    assert "esp_wifi_set_mode(WIFI_MODE_APSTA)" in manager
    assert "manual_config_timeout_cb" in manager
    assert 'esp_wifi_set_mode(WIFI_MODE_STA)' in manager
    assert "!connected || !ap_active || manual_config_active" in manager
    assert "out->manual_config_active = s_manual_config_active" in manager
    assert "out->manual_config_remaining_ms" in manager
    set_ap_active_body = manager.split("static void set_ap_active(bool active)", 1)[1].split("static esp_err_t configure_ap", 1)[0]
    assert "xTimerStop(s_manual_config_timer" not in set_ap_active_body
    assert "s_manual_config_active = false" not in set_ap_active_body
    assert "s_manual_config_deadline_us = 0" not in set_ap_active_body


def test_documentation_describes_touch_hold_and_timeout_in_both_languages() -> None:
    en = _read("docs/web-configuration.md")
    fr = _read("docs/fr/web-configuration.md")
    troubleshooting_en = _read("docs/troubleshooting.md")
    troubleshooting_fr = _read("docs/fr/troubleshooting.md")

    assert "top-left corner" in en
    assert "five seconds" in en
    assert "five minutes" in en
    assert "coin supérieur gauche" in fr
    assert "cinq secondes" in fr
    assert "cinq minutes" in fr
    assert "top-left corner" in troubleshooting_en
    assert "coin supérieur gauche" in troubleshooting_fr
