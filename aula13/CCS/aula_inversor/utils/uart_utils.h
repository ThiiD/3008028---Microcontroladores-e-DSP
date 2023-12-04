/*
 * uart_utils.h
 *
 *  Created on: 2 de nov de 2023
 *      Author: GUILHERME MÁRCIO SOARES
 */

#ifndef UTILS_UART_UTILS_HPP_
#define UTILS_UART_UTILS_HPP_
#include <stdint.h>
#include <stdbool.h>

#include "driverlib/uart.h"

void send_int(uint32_t ui32Base, int data);
void send_uint16(uint32_t ui32Base, uint16_t data);
void recv_int(uint32_t ui32Base, int* data);
void send_float(uint32_t ui32Base, float data);
void recv_float(uint32_t ui32Base, float* data);

#endif /* UTILS_UART_UTILS_HPP_ */
