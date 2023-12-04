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

#define SIGNAL_LENGTH 112

uint32_t freq_pwm = 32000;
uint32_t SystemClockFreq;
float SystemClockPeriod;
uint8_t i = 0, flagChangeDuty = 0;
uint8_t duty[SIGNAL_LENGTH];

// Interrupção para alterar o valor de pwm
void Timer0IntHandler(void)
{
    // Clear the timer interrupt
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ((SystemClockFreq/freq_pwm)*25*duty[i])/100);
    i = i + 1;                                                                                                      // Incrementa i
    i = i % 112;                                                                                                    // Faz o buffer circular
}

int main(void)
{
    uint32_t systemClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
    | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);                                                          // Define o clock do sistema (clock interno, 120 MHz)

    SystemClockFreq = SysCtlClockGet();                                                                         // Recebe a frequencia do clock interno
    SystemClockPeriod = 1/SystemClockFreq;                                                                      // Calcula o periodo do clock


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
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 25*SystemClockFreq/freq_pwm);                                            // Configura o periodo do PWM (LOAD = SystemClockFreq/freq_pwm)
    uint32_t ui32MyDuty = 25*((SystemClockFreq/freq_pwm)*50.0)/(100.0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ui32MyDuty);                                // Coloca o DutyCycle de 0.50/
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);                                                                         // Habilita o Timer do gerador 0
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);                                                             // Habilita a saida do pwm



    // ------------------------------------------ RECEBE O SINAL ------------------------------------------
    while(i < SIGNAL_LENGTH){
        if (UARTCharsAvail(UART0_BASE)){
            duty[i] = UARTCharGet(UART0_BASE);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ((SystemClockFreq/freq_pwm)*25*duty[i])/100);
            i = i+1;
        }
    }
    i = 0;

    // ------------------------------------ CONFIGURA TIMER/INTERRUPT -------------------------------------
    uint16_t ui16SignalFreq = 500;                                                                              // Frequencia do sinal recebido
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);                                                               // Habilita o periferico de timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);                                                            // Configura o timer como periodico
    uint32_t ui32Period = SystemClockFreq/(ui16SignalFreq * SIGNAL_LENGTH);                                     // Calculo do contador da interrupcao
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);                                                          // Configura o contador da interrupcao
    IntEnable(INT_TIMER0A);                                                                                     //
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);                                                            //
    IntMasterEnable();                                                                                          //
    TimerEnable(TIMER0_BASE, TIMER_A);                                                                          //

    while(1){

    }

}



