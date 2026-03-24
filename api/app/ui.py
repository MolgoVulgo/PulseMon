from pathlib import Path

_UI_INDEX = Path(__file__).parent / "ui" / "index.html"


def get_ui_html() -> str:
    return _UI_INDEX.read_text(encoding="utf-8")
