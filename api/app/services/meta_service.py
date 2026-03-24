import socket

from app.models import MetaResponse, V1_HISTORY_SERIES, V1_METRICS


def build_meta() -> MetaResponse:
    return MetaResponse(
        v=1,
        host=socket.gethostname(),
        metrics=V1_METRICS,
        history_series=V1_HISTORY_SERIES,
    )
