/*
 * r_peak.h
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include <stdint.h>

#ifndef INC_R_PEAK_H_
#define INC_R_PEAK_H_


#define RPEAK_WINDOW_SAMPLES      62
#define RPEAK_REFRACTORY_SAMPLES  100

typedef struct
{
    /* derivative stage */
    float prev_sample;
    uint8_t have_prev;

    /* moving-window integrator (circular buffer + running sum) */
    float window_buf[RPEAK_WINDOW_SAMPLES];
    uint16_t window_idx;
    float window_sum;
    uint8_t window_filled;

    /* adaptive threshold (Pan-Tompkins style peak/noise trackers) */
    float spk_i;   /* running estimate of signal (QRS) peak level */
    float npk_i;   /* running estimate of noise peak level        */
    float threshold;
    uint8_t thresholds_initialized;

    /* candidate-peak tracking while above threshold */
    uint8_t above_threshold;
    float candidate_value;
    uint32_t candidate_index;

    /* refractory / bookkeeping */
    uint32_t sample_index;
    uint32_t last_rpeak_index;
    uint8_t have_last_rpeak;

    /* stats exposed for signal-quality scoring */
    uint32_t accepted_peaks;
    uint32_t rejected_candidates; /* candidates that failed refractory/threshold checks */
} ECG_RPeakState;

void ECG_RPeak_Init(ECG_RPeakState *state);

/*
 * Feed one *filtered* ECG sample. Returns 1 on the sample where an R-peak
 * is confirmed, 0 otherwise. If it returns 1, *out_rr_samples receives the
 * RR interval (in samples) since the previous confirmed peak, or 0 if this
 * is the first detected peak.
 */
uint8_t ECG_RPeak_Process(ECG_RPeakState *state, float filtered_sample,
                           uint32_t *out_rr_samples);

#endif /* RPEAK_H */

