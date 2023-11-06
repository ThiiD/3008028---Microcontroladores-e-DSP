#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"                                                                            // Inclui a biblioteca de timer
#include "driverlib/adc.h"                                                                              // Inclui a biblioteca de adc
#include "driverlib/uart.h"                                                                             // Inclui a biblioteca de uart
#include "driverlib/udma.h"                                                                             // Inclui a biblioteca de DMA

#define SAMPLE_RATE 12000                                                                               // Define a frequencia de amostragem desejada
#define BUFFER_LENGHT 2400                                                                              // Define o tamanho do buffer (12000/2400 = 200ms)

#pragma DATA_ALIGN(pu8ControlTable, 1024);                                                              // Diretiva de compilação para a DMA
uint8_t pu8ControlTable[1024];                                                                          // Enderecos de memoria para configuracao da DMA


uint16_t bufferADC[BUFFER_LENGHT];                                                                      // Buffer para armazenar as leituras do ADC
int main(void)
{
    uint32_t systemClock = SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
            SYSCTL_CFG_VCO_480), 120000000);                                                             // Define o clock do sistema (clock interno, 120 MHz)


    // ----------------------------------------- Configura o Timer ----------------------------------------
    uint32_t myCount = systemClock / SAMPLE_RATE;                                                        // Define o valor maximo do contador para o timer

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);                                                        // Habilita o periferico de timer
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);                                                     // Seta o timer 0 como periodico
    TimerLoadSet(TIMER0_BASE, TIMER_A, myCount/2-1);                                                     // Configura o periodo do timer
    TimerEnable(TIMER0_BASE, TIMER_A);                                                                   // Inicializa o timer

    // ------------------------------------------ Configura o ADC -----------------------------------------
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);                                                          // Habilita o GPIO E
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);                                                          // Configura o pino E3 como ADC
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);                                                           // Habilita o ADC0
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);                                             // Configura o ADC para ter o Sample Sequencer 3 (1 medicao) com trigger por timer e prioridade 0
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);                                                      // Liga o timer a entrada de trigger do adc
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);                    // Configura a leitura do canal 0 e habilita interrupcao de termino de leitura ao final.
    ADCSequenceEnable(ADC0_BASE, 3);                                                                      // Habilita a sequencia 3 no ADC0

    // ------------------------------------------ Configura a DMA -----------------------------------------
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,  UDMA_SIZE_16 | UDMA_SRC_INC_NONE |
                          UDMA_DST_INC_16 | UDMA_ARB_1);                                                  // Configura o canal da DMA
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                           UDMA_MODE_BASIC,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO3),
                           &bufferADC,
                           BUFFER_LENGHT);                                                                // Configura o modo de transferencia da DMA
    ADCSequenceDMAEnable(ADC0_BASE, 3);                                                                   // Hablita a DMA para o sample sequencer 3
    ADCIntEnableEx(ADC0_BASE, ADC_INT_DMA_SS3);                                                           // Habilita a interrupcao do DMA
    IntEnable(INT_ADC0SS3);                                                                               // Habilita a interrupcao do DMA



}
