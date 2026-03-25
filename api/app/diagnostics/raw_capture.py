from __future__ import annotations

import argparse
import json

from app.diagnostics.probe import capture_gpu_raw_vs_display, capture_raw_metrics


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture raw CPU/GPU metrics for diagnostics")
    parser.add_argument("--duration-s", type=int, default=60, help="Capture duration in seconds")
    parser.add_argument("--sample-hz", type=float, default=10.0, help="Sampling frequency (Hz)")
    parser.add_argument("--ema-alpha", type=float, default=0.25, help="EMA alpha for compare mode")
    parser.add_argument(
        "--mode",
        choices=["raw", "compare"],
        default="compare",
        help="raw: all metrics raw only, compare: gpu.pct raw vs display",
    )
    parser.add_argument(
        "--output",
        type=str,
        default="api/diagnostics/raw_vs_display_gpu_pct.jsonl",
        help="Output JSONL file path",
    )
    args = parser.parse_args()

    if args.mode == "raw":
        summary = capture_raw_metrics(duration_s=args.duration_s, sample_hz=args.sample_hz, output_path=args.output)
    else:
        summary = capture_gpu_raw_vs_display(
            duration_s=args.duration_s,
            sample_hz=args.sample_hz,
            alpha=args.ema_alpha,
            output_path=args.output,
        )
    print(json.dumps(summary, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
