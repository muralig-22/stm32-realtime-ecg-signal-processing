/*
 * r_peak.c
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include "r_peak.h"

void ECG_RPeak_Init(ECG_RPeakState *state)
{
    uint16_t i;

    state->prev_sample = 0.0f;
    state->have_prev = 0;

    for (i = 0; i < RPEAK_WINDOW_SAMPLES; i++)
    {
        state->window_buf[i] = 0.0f;
    }
    state->window_idx = 0;
    state->window_sum = 0.0f;
    state->window_filled = 0;

    state->spk_i = 0.0f;
    state->npk_i = 0.0f;
    state->threshold = 0.0f;
    state->thresholds_initialized = 0;

    state->above_threshold = 0;
    state->candidate_value = 0.0f;
    state->candidate_index = 0;

    state->sample_index = 0;
    state->last_rpeak_index = 0;
    state->have_last_rpeak = 0;

    state->accepted_peaks = 0;
    state->rejected_candidates = 0;
}

uint8_t ECG_RPeak_Process(ECG_RPeakState *state, float filtered_sample,
                           uint32_t *out_rr_samples)
{
    uint8_t peak_detected = 0;
    float deriv, squared, integrated;

    if (out_rr_samples)
    {
        *out_rr_samples = 0;
    }

    /* --- 1. Derivative --- */
    if (!state->have_prev)
    {
        deriv = 0.0f;
        state->have_prev = 1;
    }
    else
    {
        deriv = filtered_sample - state->prev_sample;
    }
    state->prev_sample = filtered_sample;

    /* --- 2. Squaring --- */
    squared = deriv * deriv;

    /* --- 3. Moving-window integration (circular buffer, running sum) --- */
    state->window_sum -= state->window_buf[state->window_idx];
    state->window_buf[state->window_idx] = squared;
    state->window_sum += squared;
    state->window_idx = (uint16_t)((state->window_idx + 1) % RPEAK_WINDOW_SAMPLES);
    integrated = state->window_sum / (float)RPEAK_WINDOW_SAMPLES;

    /*
     * Bootstrap the adaptive threshold once the window has filled and
     * we have a first sense of scale, so we don't fire on transients
     * during startup.
     */
    if (!state->window_filled)
    {
        if (state->sample_index >= RPEAK_WINDOW_SAMPLES)
        {
            state->window_filled = 1;
        }
    }

    if (state->window_filled && !state->thresholds_initialized)
    {
        state->spk_i = integrated;
        state->npk_i = integrated * 0.5f;
        state->threshold = state->npk_i + 0.25f * (state->spk_i - state->npk_i);
        state->thresholds_initialized = 1;
    }

    if (state->thresholds_initialized)
    {
        if (!state->above_threshold)
        {
            if (integrated > state->threshold)
            {
                /* rising edge: start tracking a candidate peak */
                state->above_threshold = 1;
                state->candidate_value = integrated;
                state->candidate_index = state->sample_index;
            }
        }
        else
        {
            if (integrated > state->candidate_value)
            {
                state->candidate_value = integrated;
                state->candidate_index = state->sample_index;
            }

            if (integrated <= state->threshold)
            {
                /* falling edge: candidate peak is finalized */
                state->above_threshold = 0;

                uint8_t refractory_ok = 1;
                if (state->have_last_rpeak)
                {
                    uint32_t gap = state->candidate_index - state->last_rpeak_index;
                    if (gap < RPEAK_REFRACTORY_SAMPLES)
                    {
                        refractory_ok = 0;
                    }
                }

                if (refractory_ok)
                {
                    /* accept as a signal (QRS) peak */
                    state->spk_i = 0.125f * state->candidate_value + 0.875f * state->spk_i;
                    state->accepted_peaks++;

                    if (out_rr_samples && state->have_last_rpeak)
                    {
                        *out_rr_samples = state->candidate_index - state->last_rpeak_index;
                    }
                    state->last_rpeak_index = state->candidate_index;
                    state->have_last_rpeak = 1;
                    peak_detected = 1;
                }
                else
                {
                    /* too close to the last peak: treat as noise, not a real beat */
                    state->npk_i = 0.125f * state->candidate_value + 0.875f * state->npk_i;
                    state->rejected_candidates++;
                }

                state->threshold = state->npk_i + 0.25f * (state->spk_i - state->npk_i);
            }
        }
    }

    state->sample_index++;
    return peak_detected;
}



