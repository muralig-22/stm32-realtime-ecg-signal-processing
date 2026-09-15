/*
 * ecg_filter.h
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */

#ifndef INC_ECG_FILTER_H_
#define INC_ECG_FILTER_H_

typedef struct
{
    /* Direct Form II Transposed state (w[1..4]) */
    float w1, w2, w3, w4;
} ECG_FilterState;

void  ECG_Filter_Init(ECG_FilterState *state);
float ECG_Filter_Process(ECG_FilterState *state, float adc_voltage);

#endif /* INC_ECG_FILTER_H_ */
