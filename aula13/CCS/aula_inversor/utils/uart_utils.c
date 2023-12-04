/*
 * uart_utils.c
 *
 *  Created on: 2 de nov de 2023
 *      Author: guilh
 */
#include "uart_utils.h"

void send_int(uint32_t ui32Base, int data)
{
    unsigned char* ptr = (unsigned char*)&data;
    int i;
    for(i = 0; i < sizeof(data); i++)
    {
        UARTCharPut(ui32Base, *(ptr+i));
    }
}

void recv_int(uint32_t ui32Base, int* data)
{
    uint8_t i;
    unsigned char* ptr = (unsigned char*)data;

    for(i=0;i<sizeof(data);++i)
    {
        while(UARTCharsAvail(ui32Base)){}
        *ptr = UARTCharGet(ui32Base);
        ptr++;
    }
}

void send_uint16(uint32_t ui32Base, uint16_t data)
{
    unsigned char* ptr = (unsigned char*)&data;
    int i;
    for(i = 0; i < sizeof(data); i++)
    {
        UARTCharPut(ui32Base, *(ptr+i));
    }
}

void send_float(uint32_t ui32Base, float data)
{
    unsigned char* ptr = (unsigned char*)&data;
    int i;
    for(i = 0; i < sizeof(data); i++)
    {
        UARTCharPut(ui32Base, *(ptr+i));
    }
}

void recv_float(uint32_t ui32Base, float* data)
{
    uint8_t i;
    unsigned char* ptr = (unsigned char*)data;

    for(i=0;i<sizeof(data);++i)
    {
        while(UARTCharsAvail(ui32Base)){}
        *ptr = UARTCharGet(ui32Base);
        ptr++;
    }
}




