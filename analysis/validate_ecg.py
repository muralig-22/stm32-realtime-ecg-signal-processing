#!/usr/bin/env python3
"""
Offline validation plots for the streaming ECG pipeline.

Uses trace.csv produced by host/sim_ecg.exe (same C code as firmware).
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_CSV = ROOT / "data" / "ecg_dataset.csv"
DEFAULT_TRACE = ROOT / "output" / "trace.csv"
DEFAULT_UART = ROOT / "output" / "uart_log.txt"
SIM_EXE = ROOT / "host" / "sim_ecg.exe"


def run_simulator(csv_path: Path, trace_path: Path, uart_path: Path) -> None:
    if not SIM_EXE.exists():
        raise FileNotFoundError(
            f"Host simulator not found: {SIM_EXE}\n"
            "Build it first: host\\build.bat"
        )

    trace_path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            str(SIM_EXE),
            "--csv",
            str(csv_path),
            "--trace",
            str(trace_path),
            "--uart-log",
            str(uart_path),
        ],
        check=True,
        cwd=ROOT / "host",
    )


def load_inputs(csv_path: Path, trace_path: Path) -> tuple[pd.DataFrame, pd.DataFrame]:
    raw = pd.read_csv(csv_path)
    trace = pd.read_csv(trace_path)
    return raw, trace


def plot_results(raw: pd.DataFrame, trace: pd.DataFrame, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    t = trace["time_s"]

    raw_v = raw["adc_12bit"] * 3.3 / 4095.0

    fig, axes = plt.subplots(4, 1, figsize=(14, 12), sharex=True)

    axes[0].plot(t, raw_v, color="tab:blue", linewidth=0.6)
    axes[0].set_ylabel("Voltage (V)")
    axes[0].set_title("Raw ECG (from ADC)")
    axes[0].grid(True, alpha=0.3)

    axes[1].plot(t, trace["filtered"], color="tab:green", linewidth=0.6)
    axes[1].set_ylabel("Filtered")
    axes[1].set_title("Band-pass filtered ECG (5–20 Hz IIR)")
    axes[1].grid(True, alpha=0.3)

    peak_t = trace.loc[trace["r_peak"] == 1, "time_s"]
    peak_y = trace.loc[trace["r_peak"] == 1, "filtered"]
    axes[2].plot(t, trace["filtered"], color="0.7", linewidth=0.5, label="filtered")
    axes[2].plot(peak_t, peak_y, "rv", markersize=4, label="R-peaks")
    axes[2].set_ylabel("Filtered")
    axes[2].set_title("R-peak detection")
    axes[2].legend(loc="upper right")
    axes[2].grid(True, alpha=0.3)

    axes[3].plot(t, trace["bpm"], color="tab:red", linewidth=1.0)
    axes[3].set_ylabel("BPM")
    axes[3].set_xlabel("Time (s)")
    axes[3].set_title("Heart rate (median of last 5 RR intervals)")
    axes[3].grid(True, alpha=0.3)

    fig.tight_layout()
    plot_path = out_dir / "ecg_validation.png"
    fig.savefig(plot_path, dpi=150)
    plt.close(fig)

    final_bpm = trace.loc[trace["bpm"] > 0, "bpm"].iloc[-1] if (trace["bpm"] > 0).any() else 0.0
    final_quality = trace["quality_label"].iloc[-1]
    final_score = int(trace["quality_score"].iloc[-1])
    n_peaks = int(trace["r_peak"].sum())

    summary_path = out_dir / "summary.txt"
    summary_path.write_text(
        "\n".join(
            [
                "ECG validation summary",
                "========================",
                f"Samples processed : {len(trace)}",
                f"R-peaks detected  : {n_peaks}",
                f"Final BPM         : {final_bpm:.1f}",
                f"Signal quality    : {final_quality} (score {final_score}/100)",
                f"Plot saved to     : {plot_path}",
            ]
        )
        + "\n",
        encoding="utf-8",
    )

    print(summary_path.read_text(encoding="utf-8"))


def show_uart_samples(uart_path: Path, n: int = 8) -> None:
    if not uart_path.exists():
        print("No UART log found.")
        return

    lines = uart_path.read_text(encoding="utf-8").splitlines()
    print("Example UART output:")
    print("--------------------")
    for line in lines[:n]:
        print(line)
    if len(lines) > n:
        print(f"... ({len(lines) - n} more lines)")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate ECG pipeline offline")
    parser.add_argument("--csv", type=Path, default=DEFAULT_CSV)
    parser.add_argument("--trace", type=Path, default=DEFAULT_TRACE)
    parser.add_argument("--uart", type=Path, default=DEFAULT_UART)
    parser.add_argument("--skip-sim", action="store_true")
    parser.add_argument("--plots-dir", type=Path, default=ROOT / "output")
    args = parser.parse_args()

    if not args.csv.exists():
        print(f"Dataset missing: {args.csv}")
        print("Generate one: python analysis/generate_dataset.py")
        return 1

    if not args.skip_sim:
        run_simulator(args.csv, args.trace, args.uart)

    raw, trace = load_inputs(args.csv, args.trace)
    plot_results(raw, trace, args.plots_dir)
    show_uart_samples(args.uart)
    return 0


if __name__ == "__main__":
    sys.exit(main())
