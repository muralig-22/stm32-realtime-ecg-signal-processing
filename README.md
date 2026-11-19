# ECG Streaming Pipeline (STM32F401)

Real-time ECG processing at **500 Hz** on STM32, with offline validation in Python.

## Architecture

Each ADC sample is processed immediately — nothing stores the full 30-second recording.

```
ADC (500 Hz) → Band-pass IIR (5–20 Hz) → Pan-Tompkins-style QRS detector
           → RR / BPM (median of 5 intervals) → Signal quality → UART
```

| Module | File | Role |
|--------|------|------|
| Filter | `Core/Src/ecg_filter.c` | 4th-order IIR band-pass, O(1) per sample |
| R-peak | `Core/Src/r_peak.c` | Derivative → square → moving integration → adaptive threshold |
| Heart rate | `Core/Src/heart_rate.c` | Median RR, plausibility gate 30–220 BPM |
| Quality | `Core/Src/signal_quality.c` | RR stability, QRS separation, amplitude, noise ratio |
| Pipeline | `Core/Src/ecg_pipeline.c` | Wires modules for one-sample streaming |
| UART | `Core/Src/uart_protocol.c` | `$ECG,<mV×1000>,<peak>,<bpm>,<quality>*CS\r\n` |

### Filter justification

- **Type:** 4th-order IIR band-pass (Butterworth-style coefficients at 500 Hz).
- **Passband:** ~5–20 Hz — keeps QRS energy, rejects baseline wander and high-frequency noise.
- **Implementation:** Direct Form II Transposed — fixed small state, no buffer.
- **Sampling:** Coefficients designed for fs = 500 Hz (matches TIM3 trigger).

### UART format (115200 baud)

```
$ECG,<processed_mV_x1000>,<r_peak 0|1>,<bpm>,<GOOD|FAIR|POOR>*<xor_checksum>\r\n
```

Example: `$ECG,142,1,72,GOOD*3A\r\n`

Telemetry is sent every 5 samples (100 Hz) and immediately on each R-peak.

---

## Part A — Offline validation (dataset)

Use this first to verify algorithms before flashing hardware.

### 1. Prerequisites

- Python 3.10+
- GCC (MinGW / MSYS2) for the host simulator

```powershell
pip install -r analysis/requirements.txt
```

### 2. Dataset

Place the instructor CSV at `data/ecg_dataset.csv` with columns:

```
sample_index,time_s,adc_12bit
```

If you do not have the official file yet, generate a synthetic 30 s recording:

```powershell
python analysis/generate_dataset.py
```

### 3. Build host simulator

Runs the **same C code** as firmware, feeding CSV rows one at a time:

```powershell
cd host
build.bat
```

### 4. Run validation

```powershell
python analysis/validate_ecg.py
```

This will:

1. Run `host/sim_ecg.exe` on the dataset (streaming, not batch).
2. Write `output/trace.csv` and `output/uart_log.txt`.
3. Save `output/ecg_validation.png` (raw, filtered, R-peaks, BPM).
4. Print BPM, signal quality, and example UART lines.

Re-plot without re-running the simulator:

```powershell
python analysis/validate_ecg.py --skip-sim
```

Use a custom CSV path:

```powershell
python analysis/validate_ecg.py --csv path\to\your_ecg.csv
```

---

## Part B — Flash to STM32 (NUCLEO-F401RE)

### Hardware

- **ADC:** PA0 (ADC1_IN0) — ECG analog input
- **UART:** USART2 @ 115200 (PA2 TX, PA3 RX) — connect USB virtual COM port
- **Timer:** TIM3 @ 500 Hz triggers ADC conversion

### Build and flash

1. Open the project in **STM32CubeIDE**.
2. If `ecg_pipeline.c` is missing from the build, right-click project → **Refresh**, then **Build**.
3. Flash to the board (Run/Debug).

### Live UART capture

Open a serial terminal (PuTTY, Tera Term, or Cursor terminal) on the ST-LINK COM port:

- **115200**, 8N1

You should see lines like:

```
$ECG,138,0,0,POOR*...
$ECG,245,1,72,GOOD*...
```

---
<img width="1366" height="671" alt="image" src="https://github.com/user-attachments/assets/1088ae38-b2d0-49b5-8cce-b9649d62e5c9" />

<img width="840" height="504" alt="image" src="https://github.com/user-attachments/assets/bb112452-2f8b-40d9-ad12-32a40d959c34" />

## Project layout

```
ECG/
├── Core/Src/          # Firmware (filter, detector, pipeline, main)
├── Core/Inc/
├── host/              # PC simulator (same C algorithms)
├── analysis/          # Python validation & plotting
├── data/              # ecg_dataset.csv
└── output/            # trace, plots, UART log (generated)
```

---

## Signal quality criteria

Score (0–100) combines:

| Factor | Weight | Criterion |
|--------|--------|-----------|
| RR stability | 30% | Low spread in recent RR intervals |
| QRS separation | 30% | Signal vs noise peak tracker separation |
| Amplitude | 20% | Filtered peak-to-peak in expected range |
| Noise rejects | 20% | Ratio of rejected vs accepted candidates |

Labels: **GOOD** ≥ 80, **FAIR** ≥ 50, else **POOR**.
