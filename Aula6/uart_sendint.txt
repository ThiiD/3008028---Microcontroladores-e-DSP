#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"

uint32_t ui32SysClkFreq;

void UARTIntHandler(void)
{
    uint32_t ui32Status;
    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status
    UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts
    while (UARTCharsAvail(UART0_BASE)) //loop while there are chars
    {
        UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE)); //echo
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0xFF); // LEDs on
        SysCtlDelay(ui32SysClkFreq / (3 * 10)); // delay .1 sec
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0); // LEDs off
    }
}

void sendInt(int dado)
{

    char* char_ptr = (char*)&dado;
    int i;
    for(i = 0; i < sizeof(int); i++)
    {
        UARTCharPut(UART0_BASE, *(char_ptr+i));
    }
}

int readIntFromUART()
{
    int i,dado;
    char integer_bytes;
    char* char_ptr = (char*)&dado;
    for(i=0;i<4;++i)
    {
        while(UARTCharsAvail(UART0_BASE)){}
        integer_bytes = UARTCharGet(UART0_BASE);
        *char_ptr = integer_bytes;
        char_ptr++;
    }
    return dado;
}

int main(void)
{
    ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
    SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
    SYSCTL_CFG_VCO_480),
                                        120000000);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(
            UART0_BASE, ui32SysClkFreq, 115200,
            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    IntMasterEnable();
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);


    int teste;

    while (1) //let interrupt handler do the UART echo function
    {
        teste = readIntFromUART();
        sendInt(teste);
    }

}
