from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ESP_SRC = ROOT / "esp" / "src"


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def test_api_client_declares_fans_dashboard_contract() -> None:
    header = _read(ESP_SRC / "pulsemon_api_client.h")
    assert "PULSEMON_FAN_SLOT_COUNT 3" in header
    assert "typedef struct {" in header
    assert "pulsemon_fans_dashboard_t" in header
    assert "bool pulsemon_fetch_fans_dashboard" in header


def test_api_client_fetches_fans_dashboard_endpoint_and_pct_field() -> None:
    source = _read(ESP_SRC / "pulsemon_api_client.c")
    assert "%s/fans/dashboard" in source
    assert "PULSEMON_FAN_SLOT_COUNT" in source
    assert 'obj_get(item, "pct_fans")' in source
    assert "if (n > PULSEMON_FAN_SLOT_COUNT)" in source


def test_poller_routes_fan_screen_to_fans_dashboard() -> None:
    source = _read(ESP_SRC / "pulsemon_poller.c")
    assert "active_screen == SCREEN_ID_FAN" in source
    assert "pulsemon_fetch_fans_dashboard" in source
    assert "update_ui_from_fans_dashboard" in source


def test_poller_applies_visibility_policy_for_fan_panels() -> None:
    source = _read(ESP_SRC / "pulsemon_poller.c")
    assert "ui_screen_set_fans_visibility(f->slots[1].has_data, f->slots[2].has_data)" in source


def test_runtime_vars_expose_fan2_rpm_name_and_compat_wrapper() -> None:
    vars_h = _read(ESP_SRC / "vars.h")
    vars_c = _read(ESP_SRC / "vars.c")
    assert "int32_t get_var_fan_2_rpm(void);" in vars_h
    assert "void set_var_fan_2_rpm(int32_t value);" in vars_h
    assert "int32_t get_var_fan_2_rpm()" in vars_c
    assert "void set_var_fan_2_rpm(int32_t value)" in vars_c
    assert "return get_var_fan_2_rmp();" in vars_c
    assert "set_var_fan_2_rmp(value);" in vars_c


def test_ui_screen_enforces_hidden_panels_for_fan4_to_fan6() -> None:
    source = _read(ESP_SRC / "ui_screen.c")
    assert "lv_obj_add_flag(objects.fan_4, LV_OBJ_FLAG_HIDDEN);" in source
    assert "lv_obj_add_flag(objects.fan_5, LV_OBJ_FLAG_HIDDEN);" in source
    assert "lv_obj_add_flag(objects.fan_6, LV_OBJ_FLAG_HIDDEN);" in source
    assert "void ui_screen_set_fans_visibility(bool fan2_visible, bool fan3_visible)" in source
