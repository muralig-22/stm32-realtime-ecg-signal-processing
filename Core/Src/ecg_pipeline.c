/*
 * ecg_pipeline.c
 *
 * Orchestrates filter -> R-peak -> BPM -> signal quality per sample.
 */
#include "ecg_pipeline.h"

static void refresh_quality(ECG_Pipeline *pipeline)
{
    ECG_QualityLabel label;

    ECG_Quality_Evaluate(&pipeline->quality,
                         pipeline->hr.rr_history,
                         pipeline->hr.count,
                         pipeline->rpeak.spk_i,
                         pipeline->rpeak.npk_i,
                         pipeline->rpeak.accepted_peaks,
                         pipeline->rpeak.rejected_candidates,
                         &label);
    pipeline->last_quality_label = label;
}

void ECG_Pipeline_Init(ECG_Pipeline *pipeline)
{
    ECG_Filter_Init(&pipeline->filter);
    ECG_RPeak_Init(&pipeline->rpeak);
    ECG_HeartRate_Init(&pipeline->hr);
    ECG_Quality_Init(&pipeline->quality);
    pipeline->sample_count = 0;
    pipeline->last_quality_label = ECG_QUALITY_POOR;
}

ECG_PipelineOutput ECG_Pipeline_ProcessSample(ECG_Pipeline *pipeline, uint16_t adc_raw)
{
    ECG_PipelineOutput out = {0};
    float voltage;
    float filtered;
    uint32_t rr_samples = 0;
    uint8_t peak;

    voltage = ((float)adc_raw * ECG_ADC_VREF_V) / ECG_ADC_MAX_COUNTS;
    filtered = ECG_Filter_Process(&pipeline->filter, voltage);

    ECG_Quality_FeedSample(&pipeline->quality, filtered);

    peak = ECG_RPeak_Process(&pipeline->rpeak, filtered, &rr_samples);
    if (peak && rr_samples > 0)
    {
        ECG_HeartRate_Update(&pipeline->hr, rr_samples);
    }

    pipeline->sample_count++;

    if (peak || (pipeline->sample_count % 250u) == 0u)
    {
        refresh_quality(pipeline);
    }

    out.filtered = filtered;
    out.r_peak = peak;
    out.bpm_valid = pipeline->hr.bpm_valid;
    out.bpm = pipeline->hr.bpm_valid ? pipeline->hr.bpm : 0.0f;
    out.quality_label = pipeline->last_quality_label;
    out.quality_score = pipeline->quality.last_score;

    return out;
}

uint8_t ECG_Pipeline_ShouldTransmit(const ECG_Pipeline *pipeline,
                                      const ECG_PipelineOutput *out)
{
    if (out->r_peak)
    {
        return 1;
    }
    if ((pipeline->sample_count % ECG_UART_EVERY_N) == 0u)
    {
        return 1;
    }
    return 0;
}

const char *ECG_Pipeline_QualityString(ECG_QualityLabel label)
{
    switch (label)
    {
    case ECG_QUALITY_GOOD:
        return "GOOD";
    case ECG_QUALITY_FAIR:
        return "FAIR";
    default:
        return "POOR";
    }
}
