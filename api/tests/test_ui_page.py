from app.ui import get_ui_html


def test_ui_html_contains_expected_hooks() -> None:
    html = get_ui_html()

    assert "Stats Linux Monitor (V1)" in html
    assert "id=\"cpu-pct\"" in html
    assert "id=\"usage-chart\"" in html
    assert "id=\"tab-btn-fans\"" in html
    assert "id=\"tab-btn-db\"" in html
    assert "id=\"tab-fans\"" in html
    assert "id=\"tab-db\"" in html
    assert "id=\"db-reference-count\"" in html
    assert "id=\"db-reference-list\"" in html
    assert "id=\"db-reference-filter\"" in html
    assert "id=\"db-fan-manager-list\"" in html
    assert "id=\"db-fan-add\"" in html
    assert "id=\"db-fan-refresh\"" in html
    assert "id=\"fans-mode-config\"" in html
    assert "id=\"fans-config-save\"" in html
    assert "const HISTORY_WINDOW_S =" in html
    assert "const DASHBOARD_POLL_MS = 1000" in html
    assert "const HISTORY_POLL_MS = 1000" in html
    assert "const HISTORY_MODE = \"display\"" in html
    assert "fetch(`${API}/dashboard`" in html
    assert "fetch(`${API}/fans/dashboard`" in html
    assert "fetch(`${API}/fans/meta`" in html
    assert "fetch(`${API}/fans/config`" in html
    assert "fetch(`${API}/fans/reference`" in html
    assert "fetch(`${API}/db/data`" in html
    assert "fetch(`${API}/db/fans?include_deleted=1`" in html
    assert "fetch(`${API}/db/fans/${fanId}`" in html
    assert "fetch(`${API}/db/fans/${fanId}/soft-delete`" in html
    assert "fetch(`${API}/db/fans/${fanId}/restore`" in html
    assert "renderReferenceCatalog(dbReferenceItems)" in html
    assert "method: \"PUT\"" in html
    assert "pct_fans" in html
    assert "buildHistoryUrl(forceFull)" in html
    assert "params.set(\"since_ts_ms\", String(lastTs))" in html
    assert "window: String(HISTORY_WINDOW_S)" in html
    assert "mode: HISTORY_MODE" in html
    assert "Array.isArray(historyPayload?.ts_ms)" in html
