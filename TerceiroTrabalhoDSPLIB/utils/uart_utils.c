/*
 * uart_utils.c
 *
 *  Created on: 2 de nov de 2023
 *      Author: guilh
 */
#include "uart_utils.h"

void send_int(uint32_t ui32Base, int data)
{
    char* ptr = (char*)&data;
    int i;
    for(i = 0; i < sizeof(data); i++)
    {
        UARTCharPut(ui32Base, *(ptr+i));
    }
}

void recv_int(uint32_t ui32Base, int* data)
{
    uint8_t i;
    char* ptr = (char*)data;

    for(i=0;i<sizeof(data);++i)
    {
        while(UARTCharsAvail(ui32Base)){}
        *ptr = UARTCharGet(ui32Base);
        ptr++;
    }
}

void send_float(uint32_t ui32Base, float data)
{
    char* ptr = (char*)&data;
    int i;
    for(i = 0; i < sizeof(data); i++)
    {
        UARTCharPut(ui32Base, *(ptr+i));
    }
}

void recv_float(uint32_t ui32Base, float* data)
{
    uint8_t i;
    char* ptr = (char*)data;

    for(i=0;i<sizeof(data);++i)
    {
        while(UARTCharsAvail(ui32Base)){}
        *ptr = UARTCharGet(ui32Base);
        ptr++;
    }
}




