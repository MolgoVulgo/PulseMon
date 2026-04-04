from app import main as main_mod


def test_get_fans_dashboard_contract_shape() -> None:
    payload = main_mod.get_fans_dashboard().model_dump()

    assert payload["v"] == 1
    assert "ts" in payload
    assert "host" in payload
    assert "fans" in payload
    assert isinstance(payload["fans"], list)

    if payload["fans"]:
        first = payload["fans"][0]
        assert "label" in first
        assert "role" in first
        assert "rpm" in first
        assert "pwm_pct" in first


def test_get_fans_meta_contract_shape() -> None:
    payload = main_mod.get_fans_meta().model_dump()

    assert payload["v"] == 1
    assert "ts" in payload
    assert "host" in payload
    assert "channels" in payload
    assert "display_labels" in payload
    assert isinstance(payload["channels"], list)

    if payload["channels"]:
        first = payload["channels"][0]
        assert "channel" in first
        assert "hwmon_name" in first
        assert "group" in first
        assert "valid" in first
        assert "mapping" in first
