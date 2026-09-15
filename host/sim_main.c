/*
 * Host-side streaming simulator.
 *
 * Feeds the assignment CSV one sample at a time through the same C pipeline
 * used on the STM32 (no 30-second buffer).
 */
#define ECG_HOST_BUILD 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecg_pipeline.h"
#include "uart_protocol.h"

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--csv PATH] [--trace PATH] [--uart-log PATH]\n"
            "  Default csv:     ../data/ecg_dataset.csv\n"
            "  Default trace:   ../output/trace.csv\n"
            "  Default uart:    ../output/uart_log.txt\n",
            prog);
}

static int parse_csv_line(const char *line, uint32_t *idx, float *time_s, uint16_t *adc)
{
    int adc_i;

    if (line[0] == 's')
    {
        return 0;
    }

    if (sscanf(line, "%u,%f,%d", idx, time_s, &adc_i) != 3)
    {
        return 0;
    }

    if (adc_i < 0)
    {
        adc_i = 0;
    }
    if (adc_i > 4095)
    {
        adc_i = 4095;
    }

    *adc = (uint16_t)adc_i;
    return 1;
}

static void write_uart_frame(FILE *uart_log,
                             float filtered,
                             uint8_t r_peak,
                             float bpm,
                             const char *quality_label)
{
    char payload[UART_LINE_MAX_LEN];
    char frame[UART_LINE_MAX_LEN];
    int32_t processed_x1000 = (int32_t)(filtered * 1000.0f);
    int32_t bpm_rounded = (int32_t)(bpm + 0.5f);
    uint8_t cs = 0;
    int n;
    int j;

    n = snprintf(payload, sizeof(payload), "ECG,%ld,%u,%ld,%s",
                 (long)processed_x1000,
                 (unsigned)r_peak,
                 (long)bpm_rounded,
                 quality_label);
    for (j = 0; j < n; j++)
    {
        cs ^= (uint8_t)payload[j];
    }
    snprintf(frame, sizeof(frame), "$%s*%02X\r\n", payload, cs);

    if (uart_log)
    {
        fputs(frame, uart_log);
    }
    fputs(frame, stdout);
}

int main(int argc, char **argv)
{
    const char *csv_path = "../data/ecg_dataset.csv";
    const char *trace_path = "../output/trace.csv";
    const char *uart_log_path = "../output/uart_log.txt";
    FILE *csv = NULL;
    FILE *trace = NULL;
    FILE *uart_log = NULL;
    char line[256];
    ECG_Pipeline pipeline;
    uint32_t rows = 0;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--csv") == 0 && (i + 1) < argc)
        {
            csv_path = argv[++i];
        }
        else if (strcmp(argv[i], "--trace") == 0 && (i + 1) < argc)
        {
            trace_path = argv[++i];
        }
        else if (strcmp(argv[i], "--uart-log") == 0 && (i + 1) < argc)
        {
            uart_log_path = argv[++i];
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    csv = fopen(csv_path, "r");
    if (!csv)
    {
        fprintf(stderr, "Cannot open CSV: %s\n", csv_path);
        return 1;
    }

    trace = fopen(trace_path, "w");
    if (!trace)
    {
        fprintf(stderr, "Cannot open trace file: %s\n", trace_path);
        fclose(csv);
        return 1;
    }

    uart_log = fopen(uart_log_path, "w");
    if (!uart_log)
    {
        fprintf(stderr, "Warning: cannot open UART log: %s\n", uart_log_path);
    }

    ECG_Pipeline_Init(&pipeline);

    fprintf(trace,
            "sample_index,time_s,adc_12bit,voltage,filtered,r_peak,bpm,"
            "quality_score,quality_label\n");

    if (!fgets(line, sizeof(line), csv))
    {
        fprintf(stderr, "CSV is empty: %s\n", csv_path);
        fclose(csv);
        fclose(trace);
        if (uart_log) fclose(uart_log);
        return 1;
    }

    while (fgets(line, sizeof(line), csv))
    {
        uint32_t idx;
        float time_s;
        uint16_t adc;
        ECG_PipelineOutput out;
        float voltage;
        const char *quality;

        if (!parse_csv_line(line, &idx, &time_s, &adc))
        {
            continue;
        }

        out = ECG_Pipeline_ProcessSample(&pipeline, adc);
        voltage = ((float)adc * ECG_ADC_VREF_V) / ECG_ADC_MAX_COUNTS;
        quality = ECG_Pipeline_QualityString(out.quality_label);

        fprintf(trace,
                "%u,%.6f,%u,%.6f,%.6f,%u,%.2f,%u,%s\n",
                (unsigned)idx,
                time_s,
                (unsigned)adc,
                voltage,
                out.filtered,
                (unsigned)out.r_peak,
                out.bpm,
                (unsigned)out.quality_score,
                quality);

        if (ECG_Pipeline_ShouldTransmit(&pipeline, &out))
        {
            write_uart_frame(uart_log,
                             out.filtered,
                             out.r_peak,
                             out.bpm,
                             quality);
        }

        rows++;
    }

    fclose(csv);
    fclose(trace);
    if (uart_log)
    {
        fclose(uart_log);
    }

    fprintf(stderr, "Processed %u samples\n", (unsigned)rows);
    fprintf(stderr, "Trace: %s\n", trace_path);
    fprintf(stderr, "UART log: %s\n", uart_log_path);

    return 0;
}
