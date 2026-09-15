/*
 * ecg_pipeline.h
 *
 * Streaming ECG processing: one ADC sample in, metrics updated in place.
 * No full-recording buffer — only small fixed state for filter / detector.
 */
#ifndef INC_ECG_PIPELINE_H_
#define INC_ECG_PIPELINE_H_

#include <stdint.h>
#include "ecg_filter.h"
#include "r_peak.h"
#include "heart_rate.h"
#include "signal_quality.h"

#define ECG_ADC_VREF_V     3.3f
#define ECG_ADC_MAX_COUNTS 4095.0f
#define ECG_UART_EVERY_N   5u

typedef struct
{
    ECG_FilterState filter;
    ECG_RPeakState rpeak;
    ECG_HeartRateState hr;
    ECG_QualityState quality;
    uint32_t sample_count;
    ECG_QualityLabel last_quality_label;
} ECG_Pipeline;

typedef struct
{
    float filtered;
    uint8_t r_peak;
    float bpm;
    uint8_t bpm_valid;
    ECG_QualityLabel quality_label;
    uint8_t quality_score;
} ECG_PipelineOutput;

void ECG_Pipeline_Init(ECG_Pipeline *pipeline);
ECG_PipelineOutput ECG_Pipeline_ProcessSample(ECG_Pipeline *pipeline, uint16_t adc_raw);
uint8_t ECG_Pipeline_ShouldTransmit(const ECG_Pipeline *pipeline, const ECG_PipelineOutput *out);
const char *ECG_Pipeline_QualityString(ECG_QualityLabel label);

#endif /* INC_ECG_PIPELINE_H_ */
