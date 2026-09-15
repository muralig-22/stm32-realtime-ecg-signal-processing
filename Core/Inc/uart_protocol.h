/*
 * uart_protocol.h
 *
 *  Created on: Sep 15, 2026
 *      Author: sys102
 */
#include <stdint.h>


#ifndef INC_UART_PROTOCOL_H_
#define INC_UART_PROTOCOL_H_

#define UART_LINE_MAX_LEN 64

void UART_Protocol_Init(void);

/*
 * Formats and sends one telemetry line. quality_label must be one of
 * "GOOD", "FAIR", "POOR".
 */
void UART_Send_Frame(float processed_value, uint8_t r_peak,
                      float bpm, const char *quality_label);


#endif /* INC_UART_PROTOCOL_H_ */
