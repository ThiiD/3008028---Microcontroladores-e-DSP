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

#define ADC_SAMPLE_SEQ_3 3
#define BUFFER_LENGHT 27


float fl32FilterCoefs[BUFFER_LENGHT] = {0.003864509926156558, -1.4925411218469763E-05, -0.00791053061591455,
                         1.2255887397377913E-05, 0.01555949263470195, -2.875293768593527E-05,
                         -0.028210588680297216, 2.242416835528681E-05, 0.05020501979500752,
                         -3.760831172782018E-05, -0.09755306284458376, 2.943150691243952E-05,
                         0.31536774586835553, 0.4999618206673245, 0.31536774586835553,
                         2.943150691243952E-05, -0.09755306284458376, -3.760831172782018E-05,
                         0.05020501979500752, 2.242416835528681E-05, -0.028210588680297216,
                         -2.875293768593527E-05, 0.01555949263470195, 1.2255887397377913E-05,
                         -0.00791053061591455, -1.4925411218469763E-05, 0.003864509926156558};
uint32_t ui32InputSignal[BUFFER_LENGHT];
uint8_t ui8Counter = 0;
uint32_t ui32MyDuty;
uint32_t ui32SystemClock, ui32PWMFreq, ui32SampleRate, ui32ADCSampleRate;
uint32_t ui32OutputSignal;

void ADCIntHandler(){
    ADCIntClear(ADC0_BASE, ADC_SAMPLE_SEQ_3);
    ADCSequenceDataGet(ADC0_BASE, ADC_SAMPLE_SEQ_3, &ui32InputSignal[ui8Counter] );
    ui32OutputSignal = 0;
    for(int i=0; i<BUFFER_LENGHT; i++){
        ui32OutputSignal += (float)(ui32InputSignal[(ui8Counter + i)%BUFFER_LENGHT])*fl32FilterCoefs[i];
    }
    ui32MyDuty = ui32SystemClock/ui32PWMFreq;
    ui32MyDuty = (float)(ui32MyDuty)*((float)ui32OutputSignal/4095.0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ui32MyDuty);
    ui8Counter++;
    ui8Counter %= BUFFER_LENGHT;
}

int main(void)
{
    ui32SystemClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
    | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);                                                             // Define o clock do sistema (clock interno, 120 MHz)

//    uint32_t ui32SystemPeriod = 1/ui32SystemClock;                                                                // Calcula o periodo do clock
    ui32PWMFreq = 50000;                                                                                            // Frequencia para o PWM de saida
    ui32ADCSampleRate = 5000;                                                                                       // Frequencia de amostragem do adc

    // ------------------------------------------ Configura a UART -----------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);                                                                    // Inicializa o periferio de UART
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){ }                                                           // Espera ate que o periferico esteja habilitado
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);                                                                    // Inicializa o GPIO A
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){ }                                                           // Espera ate que o GPIO esteja habilitado
    GPIOPinConfigure(GPIO_PA0_U0RX);                                                                                // Configuracao alternativa. Configura o pino PA0 como RX
    GPIOPinConfigure(GPIO_PA1_U0TX);                                                                                // Configuracao alternativa. Configura o pino PA1 como TX
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);                                                      // Configura os pinos para serem utilizados pela UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);                                                                    // Inicializa o GPIO N
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);                                                // Configura os pinos 0 e 1 para serem utilizados como output
    UARTConfigSetExpClk(
            UART0_BASE, ui32SystemClock, 115200,
            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));                                    // Configura o periferico de uart: baudrate, stopbit e paridade.

    // --------------------------------------- CONFIGURA PWM DE SAIDA------------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);                                                                    // Habilita o periferico de GPIOF
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));                                                             // Espera habilitar o GPIOF
    GPIOPinConfigure(GPIO_PF1_M0PWM1);                                                                              // Configura o pino PF1 para PWM1
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);                                                                    // Configura o pino PF1 para PWM1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);                                                                     // Habilita o periferico de pwm
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));                                                              // Verifica e espera ate habilitar o PWM1
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);                                // Configura o modulo 0, gerador de PWM 0, para trabalhar no modo dowm
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ui32SystemClock/ui32PWMFreq);                                             // Configura o periodo do PWM (LOAD = SystemClockFreq/freq_pwm)
    ui32MyDuty = ((ui32SystemClock/ui32PWMFreq)*50.0)/(100.0);                                                      // Define uma variavel para teste no periferico de pwm
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, ui32MyDuty);                                                             // Coloca o DutyCycle de 0.50/
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);                                                                             // Habilita o Timer do gerador 0
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);                                                                 // Habilita a saida do pwm

    // ------------------------------------------ Configura o ADC -----------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);                                                                    // Habilita o GPIO E
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)){ }
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);                                                                    // Configura o pino E3 como ADC

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);                                                                     // Habilita o ADC0
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)){ }
    IntDisable(INT_ADC0SS3);
    ADCIntDisable(ADC0_BASE, ADC_SAMPLE_SEQ_3);
    ADCSequenceDisable(ADC0_BASE, ADC_SAMPLE_SEQ_3);
    ADCSequenceConfigure(ADC0_BASE, ADC_SAMPLE_SEQ_3, ADC_TRIGGER_TIMER, 0);                                        // Configura o ADC para ter o Sample Sequencer 3 (1 medicao) com trigger por timer e prioridade 0                                                   // Liga o timer a entrada de trigger do adc
    ADCSequenceStepConfigure(ADC0_BASE, ADC_SAMPLE_SEQ_3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);               // Configura a leitura do canal 0 e habilita interrupcao de termino de leitura ao final.
    ADCSequenceEnable(ADC0_BASE, ADC_SAMPLE_SEQ_3);                                                                 // Habilita a sequencia 3 no ADC0
    ADCIntClear(ADC0_BASE, ADC_SAMPLE_SEQ_3);
    ADCIntEnable(ADC0_BASE, ADC_SAMPLE_SEQ_3);
    IntEnable(INT_ADC0SS3);

    // ----------------------------------------- Configura o Timer ----------------------------------------
    uint32_t ui32TIMERLoad = ui32SystemClock / ui32ADCSampleRate;                                                   // Define o valor maximo do contador para o timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);                                                                   // Habilita o periferico de timer
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)){ }                                                          // Verifica e espera ate q o periferico esteja habilitado
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);                                                                // Seta o timer 0 como periodico
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);                                                                // Habilita o timer como trigger pro ADC
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32TIMERLoad/2-1);                                                          // Configura o periodo do timer
    IntMasterEnable();                                                                                              // Inicializa o timer
    TimerEnable(TIMER0_BASE , TIMER_A);


    while(1){

    }

}



