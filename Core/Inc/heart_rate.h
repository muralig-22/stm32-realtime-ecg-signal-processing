/*
 * heart_rate.h
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include <stdint.h>

#ifndef INC_HEART_RATE_H_
#define INC_HEART_RATE_H_


#define HR_HISTORY_LEN       5
#define HR_FS_HZ              500u
#define HR_MIN_PLAUSIBLE_BPM  30u
#define HR_MAX_PLAUSIBLE_BPM  220u

typedef struct
{
    uint32_t rr_history[HR_HISTORY_LEN]; /* in samples */
    uint8_t count;      /* number of valid entries currently stored (<= HR_HISTORY_LEN) */
    uint8_t next_slot;
    float bpm;           /* last stabilized BPM, 0 until enough data exists */
    uint8_t bpm_valid;
} ECG_HeartRateState;

void  ECG_HeartRate_Init(ECG_HeartRateState *state);

/*
 * Feed a new RR interval (in samples). Returns 1 and updates state->bpm
 * if the interval was plausible and accepted; returns 0 if it was
 * rejected as implausible (missed beat / false peak) and BPM is
 * unchanged.
 */
uint8_t ECG_HeartRate_Update(ECG_HeartRateState *state, uint32_t rr_samples);

#endif /* HEART_RATE_H */

