/*
 * signal_quality.c
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include "signal_quality.h"

/* Plausible filtered-ECG peak-to-peak amplitude range (arbitrary but
   consistent volt-ish units after the bandpass filter). Tune against
   your own hardware/dataset -- see analysis/analyze_ecg.py. */
#define QUALITY_AMP_MIN_EXPECTED  0.02f
#define QUALITY_AMP_MAX_EXPECTED  2.00f

/* Amplitude tracker decay per sample (slow leak so min/max adapt to a
   changing signal without needing a full history buffer). */
#define QUALITY_AMP_DECAY 0.999f

void ECG_Quality_Init(ECG_QualityState *state)
{
    state->amp_min = 0.0f;
    state->amp_max = 0.0f;
    state->amp_initialized = 0;
    state->last_score = 0;
    state->last_label = ECG_QUALITY_POOR;
}

void ECG_Quality_FeedSample(ECG_QualityState *state, float filtered_sample)
{
    if (!state->amp_initialized)
    {
        state->amp_min = filtered_sample;
        state->amp_max = filtered_sample;
        state->amp_initialized = 1;
        return;
    }

    if (filtered_sample > state->amp_max)
    {
        state->amp_max = filtered_sample;
    }
    else
    {
        state->amp_max = state->amp_max * QUALITY_AMP_DECAY;
    }

    if (filtered_sample < state->amp_min)
    {
        state->amp_min = filtered_sample;
    }
    else
    {
        state->amp_min = state->amp_min * QUALITY_AMP_DECAY;
    }
}

static uint8_t rr_stability_score(const uint32_t *rr, uint8_t n)
{
    if (n < 2)
    {
        return 0; /* not enough data yet */
    }

    uint32_t min_rr = rr[0], max_rr = rr[0];
    uint8_t i;
    for (i = 1; i < n; i++)
    {
        if (rr[i] < min_rr) min_rr = rr[i];
        if (rr[i] > max_rr) max_rr = rr[i];
    }
    if (min_rr == 0) return 0;

    /* Relative spread of RR intervals: 0 = perfectly regular. */
    float spread = (float)(max_rr - min_rr) / (float)min_rr;

    if (spread <= 0.08f) return 100;
    if (spread >= 0.50f) return 0;
    /* linear falloff between 8% and 50% spread */
    return (uint8_t)(100.0f * (0.50f - spread) / (0.50f - 0.08f));
}

static uint8_t qrs_consistency_score(float spk_i, float npk_i)
{
    float denom = spk_i + npk_i;
    if (denom <= 0.0f)
    {
        return 0;
    }
    float separation = (spk_i - npk_i) / denom; /* -1..1, want close to 1 */
    if (separation < 0.0f) separation = 0.0f;
    return (uint8_t)(separation * 100.0f);
}

static uint8_t amplitude_score(float amp_min, float amp_max)
{
    float pp = amp_max - amp_min;
    if (pp <= 0.0f) return 0;

    if (pp < QUALITY_AMP_MIN_EXPECTED)
    {
        /* too flat: probably lead-off / poor contact */
        return (uint8_t)(100.0f * (pp / QUALITY_AMP_MIN_EXPECTED));
    }
    if (pp > QUALITY_AMP_MAX_EXPECTED)
    {
        /* saturating / motion artifact */
        float excess = (pp - QUALITY_AMP_MAX_EXPECTED) / QUALITY_AMP_MAX_EXPECTED;
        float score = 100.0f - excess * 100.0f;
        if (score < 0.0f) score = 0.0f;
        return (uint8_t)score;
    }
    return 100; /* within the healthy range */
}

static uint8_t noise_score(uint32_t accepted, uint32_t rejected)
{
    uint32_t total = accepted + rejected;
    if (total == 0)
    {
        return 50; /* no data yet: neutral */
    }
    float reject_ratio = (float)rejected / (float)total;
    float score = 100.0f * (1.0f - reject_ratio);
    if (score < 0.0f) score = 0.0f;
    return (uint8_t)score;
}

uint8_t ECG_Quality_Evaluate(ECG_QualityState *state,
                              const uint32_t *rr_history, uint8_t rr_count,
                              float spk_i, float npk_i,
                              uint32_t accepted_recent, uint32_t rejected_recent,
                              ECG_QualityLabel *out_label)
{
    uint8_t s_rr    = rr_stability_score(rr_history, rr_count);
    uint8_t s_qrs   = qrs_consistency_score(spk_i, npk_i);
    uint8_t s_amp   = amplitude_score(state->amp_min, state->amp_max);
    uint8_t s_noise = noise_score(accepted_recent, rejected_recent);

    float score_f = 0.30f * s_rr + 0.30f * s_qrs + 0.20f * s_amp + 0.20f * s_noise;
    uint8_t score = (uint8_t)(score_f + 0.5f);

    ECG_QualityLabel label;
    if (score >= 80)      label = ECG_QUALITY_GOOD;
    else if (score >= 50) label = ECG_QUALITY_FAIR;
    else                  label = ECG_QUALITY_POOR;

    state->last_score = score;
    state->last_label = label;
    if (out_label) *out_label = label;

    return score;
}


