from app.ui import get_ui_html


def test_ui_html_contains_expected_hooks() -> None:
    html = get_ui_html()

    assert "Stats Linux Monitor (V1)" in html
    assert "id=\"cpu-pct\"" in html
    assert "id=\"usage-chart\"" in html
    assert "fetch(`${API}/dashboard`" in html
    assert "fetch(`${API}/history?window=300&step=1`" in html
