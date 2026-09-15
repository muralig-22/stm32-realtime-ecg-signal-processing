/*
 * ecg_filter.c
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include "ecg_filter.h"

static const float B0 =  0.0036216815f;
static const float B1 =  0.0000000000f;
static const float B2 = -0.0072433630f;
static const float B3 =  0.0000000000f;
static const float B4 =  0.0036216815f;

static const float A1 = -3.8000503653f;
static const float A2 =  5.4393397878f;
static const float A3 = -3.4763426472f;
static const float A4 =  0.8371816513f;

void ECG_Filter_Init(ECG_FilterState *state)
{
    state->w1 = 0.0f;
    state->w2 = 0.0f;
    state->w3 = 0.0f;
    state->w4 = 0.0f;
}

/* Direct Form II Transposed, one sample in -> one sample out, O(1) work. */
float ECG_Filter_Process(ECG_FilterState *state, float x)
{
    float y = B0 * x + state->w1;

    state->w1 = B1 * x - A1 * y + state->w2;
    state->w2 = B2 * x - A2 * y + state->w3;
    state->w3 = B3 * x - A3 * y + state->w4;
    state->w4 = B4 * x - A4 * y;

    return y;
}
