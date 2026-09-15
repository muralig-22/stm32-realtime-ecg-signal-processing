/*
 * uart_protocol.c
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include "uart_protocol.h"
#include <stdio.h>
#include <string.h>


#if defined(USE_HAL_DRIVER) && !defined(ECG_HOST_BUILD)
#define ECG_TARGET_STM32 1
#else
#define ECG_TARGET_STM32 0
#endif

#if ECG_TARGET_STM32
#include "main.h" /* for huart2, or whichever handle CubeMX generated */
extern UART_HandleTypeDef huart2;
#endif

static uint8_t compute_checksum(const char *payload_after_dollar)
{
    uint8_t cs = 0;
    while (*payload_after_dollar)
    {
        cs ^= (uint8_t)(*payload_after_dollar);
        payload_after_dollar++;
    }
    return cs;
}

static void low_level_send(const char *line, uint16_t len)
{
#if ECG_TARGET_STM32
    HAL_UART_Transmit(&huart2, (uint8_t *)line, len, 100);
    /* For non-blocking transmission instead, use:
       HAL_UART_Transmit_IT(&huart2, (uint8_t *)line, len);
       or HAL_UART_Transmit_DMA(...) with a small ring buffer if the
       application needs to keep sampling while a previous frame is
       still going out. */
#else
    fwrite(line, 1, len, stdout);
#endif
}

void UART_Protocol_Init(void)
{
    /* Nothing to do for the host build. On STM32 this is where you'd
       confirm huart2 is already MX_USARTx_UART_Init()'d by CubeMX. */
}

void UART_Send_Frame(float processed_value, uint8_t r_peak,
                      float bpm, const char *quality_label)
{
    char payload[UART_LINE_MAX_LEN];
    char line[UART_LINE_MAX_LEN];
    int32_t processed_x1000 = (int32_t)(processed_value * 1000.0f);
    int32_t bpm_rounded = (int32_t)(bpm + 0.5f);

    /* Build the part between '$' and '*' first so we can checksum it. */
    snprintf(payload, sizeof(payload), "ECG,%ld,%u,%ld,%s",
              (long)processed_x1000, (unsigned)r_peak,
              (long)bpm_rounded, quality_label);

    uint8_t cs = compute_checksum(payload);

    int len = snprintf(line, sizeof(line), "$%s*%02X\r\n", payload, cs);
    if (len > 0)
    {
        low_level_send(line, (uint16_t)len);
    }
}


