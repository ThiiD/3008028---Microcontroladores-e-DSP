#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"                                                                            // Inclui a biblioteca de timer
#include "driverlib/interrupt.h"                                                                        // Inclui a biblioteca de interrupt
#include "driverlib/adc.h"                                                                              // Inclui a biblioteca de adc
#include "driverlib/uart.h"                                                                             // Inclui a biblioteca de uart
#include "driverlib/udma.h"                                                                             // Inclui a biblioteca de DMA
#include "driverlib/pwm.h"                                                                              // Inclui a biblioteca de PWM

#define SIGNAL_LENGHT 112

uint16_t freq_pwm = 2000;
uint32_t SystemClockFreq, SystemClockPeriod;
uint8_t i = 0, flagChangeDuty = 0;
uint8_t duty[SIGNAL_LENGHT];

int main(void)
{
    uint32_t systemClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
    | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);                                                          // Define o clock do sistema (clock interno, 120 MHz)

    SystemClockFreq = SysCtlClockGet();                                                                         // Recebe a frequencia do clock interno
    // SystemClockPeriod = 1/SystemClockFreq;                                                                   // Calcula o periodo do clock


    // ------------------------------------------ Configura a UART -----------------------------------------
        SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){ }
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){ }
        GPIOPinConfigure(GPIO_PA0_U0RX);
        GPIOPinConfigure(GPIO_PA1_U0TX);
        GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
        GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);
        UARTConfigSetExpClk(
                UART0_BASE, systemClock, 115200,
                (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // ------------------------------------------ CONFIGURA PWM ------------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);                                                                // Habilita o periferico de GPIOF
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));                                                         // Espera habilitar o GPIOF
    GPIOPinConfigure(GPIO_PF1_M0PWM1);                                                                          // Configura o pino PF1 para PWM1
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);                                                                // Configura o pino PF1 para PWM1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);                                                                 // Habilita o periferico de pwm
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));                                                          // Verifica e espera ate habilitar o PWM1
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);                            // Configura o modulo 0, gerador de PWM 0, para trabalhar no modo dowm
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, SystemClockFreq/freq_pwm);                                            // Configura o periodo do PWM (LOAD = SystemClockFreq/freq_pwm)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ((SystemClockFreq/freq_pwm)*50)/100);                                // Coloca o DutyCycle de 0.50
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);                                                                         // Habilita o Timer do gerador 0
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);                                                             // Habilita a saida do pwm



    while(i < SIGNAL_LENGHT){
        if (UARTCharsAvail(UART0_BASE)){
            duty[i] = UARTCharGet(UART0_BASE);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ((SystemClockFreq/freq_pwm)*duty[i])/100);
            i = i+1;
        }
    }

    while(1){
        UARTCharPut(UART0_BASE, "o");
        UARTCharPut(UART0_BASE, "k");
        UARTCharPut(UART0_BASE, "\n");
    }
}


// Interrupção para alterar o valor de pwm
void TimerIntHandler(){
    if(flagChangeDuty == 1){
        flagChangeDuty = 0;
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (SystemClockFreq/freq_pwm*duty[i])/100);

    }
}

// COLOCAR A UART E O TIMER DEPOIS


