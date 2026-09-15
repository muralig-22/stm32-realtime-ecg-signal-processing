#!/usr/bin/env bash
# Run this from MSYS2 UCRT64 shell:  ./build.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$(dirname "$0")/sim_ecg.exe"

gcc -std=c11 -O2 -DECG_HOST_BUILD \
  -I"$ROOT/Core/Inc" \
  "$ROOT/Core/Src/ecg_filter.c" \
  "$ROOT/Core/Src/r_peak.c" \
  "$ROOT/Core/Src/heart_rate.c" \
  "$ROOT/Core/Src/signal_quality.c" \
  "$ROOT/Core/Src/uart_protocol.c" \
  "$ROOT/Core/Src/ecg_pipeline.c" \
  "$(dirname "$0")/sim_main.c" \
  -o "$OUT"

echo "Built: $OUT"
