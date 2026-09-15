#!/usr/bin/env python3
"""
Generate a 30 s / 500 Hz synthetic ECG CSV for offline validation.

Replace data/ecg_dataset.csv with the instructor-supplied file when available.
Columns: sample_index, time_s, adc_12bit
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

FS = 500
DURATION_S = 30
N = FS * DURATION_S
VREF = 3.3
ADC_MAX = 4095
BPM = 72.0


def qrs_pulse(t: float, width: float = 0.012) -> float:
    """Simple bipolar QRS-like waveform centred at t=0."""
    x = t / width
    return 1.6 * x * math.exp(-x * x)


def synth_voltage(t: float, beat_period: float, noise: float) -> float:
    phase = t % beat_period
    beat_center = 0.08
    signal = qrs_pulse(phase - beat_center)
    signal += 0.15 * qrs_pulse(phase - beat_center - 0.04, width=0.020)
    baseline = 0.25 * math.sin(2 * math.pi * 0.3 * t)
    signal += baseline + noise
    return 1.65 + signal


def voltage_to_adc(v: float) -> int:
    counts = int(round(v / VREF * ADC_MAX))
    return max(0, min(ADC_MAX, counts))


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate synthetic ECG CSV")
    parser.add_argument(
        "--out",
        type=Path,
        default=Path(__file__).resolve().parent.parent / "data" / "ecg_dataset.csv",
    )
    parser.add_argument("--bpm", type=float, default=BPM)
    args = parser.parse_args()

    args.out.parent.mkdir(parents=True, exist_ok=True)
    beat_period = 60.0 / args.bpm
    rng_state = 12345

    def rand_uniform() -> float:
        nonlocal rng_state
        rng_state = (1103515245 * rng_state + 12345) & 0x7FFFFFFF
        return (rng_state / 0x7FFFFFFF) * 2.0 - 1.0

    with args.out.open("w", encoding="utf-8", newline="\n") as f:
        f.write("sample_index,time_s,adc_12bit\n")
        for i in range(N):
            t = i / FS
            noise = 0.015 * rand_uniform()
            v = synth_voltage(t, beat_period, noise)
            adc = voltage_to_adc(v)
            f.write(f"{i},{t:.6f},{adc}\n")

    print(f"Wrote {N} samples to {args.out}")
    print(f"Target heart rate: {args.bpm:.1f} BPM")


if __name__ == "__main__":
    main()
