/*
 * heart_rate.c
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include "heart_rate.h"

static uint32_t rr_to_bpm_x1000(uint32_t rr_samples)
{
    /* BPM = 60 * fs / RR_samples, scaled by 1000 to avoid float here */
    return (60000u * HR_FS_HZ) / rr_samples;
}

static uint32_t median_of_history(const uint32_t *arr, uint8_t n)
{
    /* n <= HR_HISTORY_LEN (small), simple insertion-sort copy is fine
       and keeps this allocation-free. */
    uint32_t sorted[HR_HISTORY_LEN];
    uint8_t i, j;

    for (i = 0; i < n; i++)
    {
        sorted[i] = arr[i];
    }
    for (i = 1; i < n; i++)
    {
        uint32_t key = sorted[i];
        j = i;
        while (j > 0 && sorted[j - 1] > key)
        {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = key;
    }
    return sorted[n / 2];
}

void ECG_HeartRate_Init(ECG_HeartRateState *state)
{
    uint8_t i;
    for (i = 0; i < HR_HISTORY_LEN; i++)
    {
        state->rr_history[i] = 0;
    }
    state->count = 0;
    state->next_slot = 0;
    state->bpm = 0.0f;
    state->bpm_valid = 0;
}

uint8_t ECG_HeartRate_Update(ECG_HeartRateState *state, uint32_t rr_samples)
{
    if (rr_samples == 0)
    {
        return 0;
    }

    uint32_t bpm_x1000 = rr_to_bpm_x1000(rr_samples);

    /* Plausibility gate: reject missed-peak (too slow) / false-peak
       (too fast) intervals before they contaminate the history. */
    if (bpm_x1000 < (uint32_t)HR_MIN_PLAUSIBLE_BPM * 1000u ||
        bpm_x1000 > (uint32_t)HR_MAX_PLAUSIBLE_BPM * 1000u)
    {
        return 0;
    }

    state->rr_history[state->next_slot] = rr_samples;
    state->next_slot = (uint8_t)((state->next_slot + 1) % HR_HISTORY_LEN);
    if (state->count < HR_HISTORY_LEN)
    {
        state->count++;
    }

    uint32_t median_rr = median_of_history(state->rr_history, state->count);
    state->bpm = (float)rr_to_bpm_x1000(median_rr) / 1000.0f;
    state->bpm_valid = 1;

    return 1;
}
