/*
 * signal_quality.h
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */

#include <stdint.h>

#ifndef INC_SIGNAL_QUALITY_H_
#define INC_SIGNAL_QUALITY_H_

typedef enum
{
    ECG_QUALITY_POOR = 0,
    ECG_QUALITY_FAIR = 1,
    ECG_QUALITY_GOOD = 2
} ECG_QualityLabel;

typedef struct
{
    float amp_min;
    float amp_max;
    uint8_t amp_initialized;
    uint8_t last_score;
    ECG_QualityLabel last_label;
} ECG_QualityState;

void ECG_Quality_Init(ECG_QualityState *state);

/* Call every sample with the *filtered* ECG value to track amplitude. */
void ECG_Quality_FeedSample(ECG_QualityState *state, float filtered_sample);

/*
 * Call periodically (e.g. once per confirmed beat, or every N samples)
 * with the current RR history and detector statistics to recompute the
 * score. Returns the 0-100 score; *out_label receives the GOOD/FAIR/POOR
 * band.
 */
uint8_t ECG_Quality_Evaluate(ECG_QualityState *state,
                              const uint32_t *rr_history, uint8_t rr_count,
                              float spk_i, float npk_i,
                              uint32_t accepted_recent, uint32_t rejected_recent,
                              ECG_QualityLabel *out_label);


#endif /* INC_SIGNAL_QUALITY_H_ */
