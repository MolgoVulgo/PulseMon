from __future__ import annotations

from collections import deque
import statistics


class MedianEmaFilter:
    """Median prefilter (N=3) followed by EMA smoothing."""

    def __init__(self, *, alpha: float = 0.25, median_window: int = 3) -> None:
        if not (0.0 < alpha <= 1.0):
            raise ValueError("alpha must be in (0, 1]")
        if median_window < 1:
            raise ValueError("median_window must be >= 1")

        self._alpha = alpha
        self._window = deque(maxlen=median_window)
        self._ema: float | None = None

    def update(self, value: float | None, *, valid: bool) -> float | None:
        if not valid or value is None:
            return None

        self._window.append(float(value))
        median_value = float(statistics.median(self._window))

        if self._ema is None:
            self._ema = median_value
        else:
            self._ema = (self._alpha * median_value) + ((1.0 - self._alpha) * self._ema)

        return self._ema

    def reset(self) -> None:
        self._window.clear()
        self._ema = None
