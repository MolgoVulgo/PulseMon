from app.ui import get_ui_html


def test_ui_html_contains_expected_hooks() -> None:
    html = get_ui_html()

    assert "Stats Linux Monitor (V1)" in html
    assert 'id="cpu-pct"' in html
    assert 'id="usage-chart"' in html
    assert 'id="tab-btn-main"' in html
    assert 'id="tab-btn-gpu"' in html
    assert 'id="tab-main"' in html
    assert 'id="tab-gpu"' in html
    assert 'id="gpu2-fan"' in html
    assert "const HISTORY_WINDOW_S =" in html
    assert "const DASHBOARD_POLL_MS = 1000" in html
    assert "const HISTORY_POLL_MS = 1000" in html
    assert 'const HISTORY_MODE = "display"' in html
    assert "fetch(`${API}/dashboard`" in html
    assert "fetch(`${API}/gpu/dashboard`" in html
    assert "buildHistoryUrl(forceFull)" in html
    assert 'params.set("since_ts_ms", String(lastTs))' in html
    assert 'window: String(HISTORY_WINDOW_S)' in html
    assert "mode: HISTORY_MODE" in html
    assert "Array.isArray(historyPayload?.ts_ms)" in html


def test_ui_html_excludes_removed_fan_and_sqlite_admin() -> None:
    html = get_ui_html()

    for removed in (
        'id="tab-btn-fans"',
        'id="tab-btn-db"',
        'id="tab-fans"',
        'id="tab-db"',
        "/fans/dashboard",
        "/fans/meta",
        "/fans/config",
        "/fans/reference",
        "/db/data",
        "/db/fans",
        "/user/config",
        "pct_fans",
        "SQLite",
    ):
        assert removed not in html
