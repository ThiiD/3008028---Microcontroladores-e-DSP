#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "inc/hw_timer.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/udma.h"
#include "driverlib/fpu.h"
#include "arm_math.h"       // biblioteca dsp

#define ENABLE_PERIPH(X) SysCtlPeripheralEnable(X); \
        while(!SysCtlPeripheralReady(X))

#define PWM_MATCH_GET(DUTY_PERCENT) TimerLoadGet(TIMER1_BASE, TIMER_A) \
                                    *(100-DUTY_PERCENT)/100 - 1;

#define ADC_SAMPLING_FREQ   5E3     // Frequência de amostragem do ADC
#define ADC_BUFFER_SIZE     64      // Tamanho do buffer de amostras do ADC
#define FILTER_ORDER        22      // Orderm do filtro utilizado

#define BLOCK_SIZE          64
#define NUM_TAPS            23
#define NUM_TAPS_ARRAY_SIZE 23

// Protótipos de funções
void timer0Init();          // Inicialização do timer 0
void timer1Init();          // Inicialização do timer 1
void adc0Init();            // Inicialização do ADC 0
void udmaInit();            // Inicialização da uDMA

void adc0uDMAConfig();      // Config. ADC0 para utilizar DMA
void timer1DMAConfig();     // Config. timer 1 para utilizar DMA

float filter(uint32_t next_sample); // função de filtragem

// Protótipos das rotinas de interrupção
void adc0s0IntHandler();    // Interrupção ADC0
void timer1BIntHandler();   // Interrupção do Timer 1B

// Tabela de controle DMA
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];

// Estados do buffer
enum ADC_BUFFER_STATUS{
    INPUT_EMPTY,
    INPUT_FILLING,
    INPUT_FULL,
};

// Variáveis globais
uint32_t sysClkFreq = 120E6;            // Frequência de clock
uint32_t PWMLoad;                       // Valor de Load do PWM
uint32_t adcBuffer1[ADC_BUFFER_SIZE];   // Buffer 1 DMA ADC0
uint32_t adcBuffer2[ADC_BUFFER_SIZE];   // Buffer 2 DMA ADC0
uint32_t outputBuffer1[ADC_BUFFER_SIZE];    // Buffer 1 de saída
uint32_t outputBuffer2[ADC_BUFFER_SIZE];    // Buffer 2 de saída
uint32_t outputBufferTest[ADC_BUFFER_SIZE]; // Buffer de teste
enum ADC_BUFFER_STATUS adcBufStatus[2]; // Status dos buffers de entrada

// Variáveis para filtragem
float32_t adcBufferF1[ADC_BUFFER_SIZE];
float32_t adcBufferF2[ADC_BUFFER_SIZE];
float32_t filteredBuffer1[ADC_BUFFER_SIZE];
float32_t filteredBuffer2[ADC_BUFFER_SIZE];
static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];
uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = ADC_BUFFER_SIZE / BLOCK_SIZE;

const float filter_coefs[] = {
                                  -0.0011395582650547264,
                                  -0.004194533851506858,
                                  -0.009231760561959188,
                                  -0.01487976703384357,
                                  -0.016982165820492424,
                                  -0.010247847170080456,
                                  0.010325575778622021,
                                  0.04579607750078735,
                                  0.09188343109499163,
                                  0.1385578710840595,
                                  0.1735771893978411,
                                  0.18653294471084267,
                                  0.1735771893978411,
                                  0.1385578710840595,
                                  0.09188343109499163,
                                  0.04579607750078735,
                                  0.010325575778622021,
                                  -0.010247847170080456,
                                  -0.016982165820492424,
                                  -0.01487976703384357,
                                  -0.009231760561959188,
                                  -0.004194533851506858,
                                  -0.0011395582650547264,
};

int main(void)
{
    // Configurando clock do sistema
    sysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                    SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                    120000000);

    // Configuração inicial dos buffers
    adcBufStatus[0] = INPUT_FILLING;
    adcBufStatus[1] = INPUT_EMPTY;

    // Desabilitando interrupções
    IntMasterDisable();

    // Habilitando FPU para processar floats
    FPUEnable();
    FPULazyStackingEnable();

    // Configuração inicial dos periféricos
    timer0Init();
    timer1Init();

    // buffer de teste
    for(uint8_t i = 0; i < ADC_BUFFER_SIZE; i++){
        outputBufferTest[i] = PWMLoad*i/ADC_BUFFER_SIZE;
    }

    adc0Init();
    udmaInit();

    // Configurando DMA
    adc0uDMAConfig();
    timer1DMAConfig();

    // Habilitando todas interrupções
    IntMasterEnable();


    static arm_fir_instance_f32 S;
    arm_fir_init_f32(&S, NUM_TAPS, (float32_t*) filter_coefs,
                     firStateF32, blockSize);

    float32_t adcData = 0;  // Variável para processar dados

    // Loop infinito
    while(1){
        // Verificando se os buffers estão cheios
        if(adcBufStatus[0] == INPUT_FULL){

            for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++)
                adcBufferF1[i] = (float32_t) adcBuffer1[i];

            for(uint32_t i = 0; i < numBlocks; i++){
                arm_fir_f32(&S,
                            adcBufferF1 + (i * blockSize),
                            filteredBuffer1 + (i * blockSize), blockSize);
            }

            for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++){
                adcBuffer1[i] = 0;
                adcData = filteredBuffer1[i];
                adcData *= (float32_t)(PWMLoad/4095);
                outputBuffer1[i] = (uint32_t)(adcData);
            }


            adcBufStatus[0] = INPUT_EMPTY;

            // Reconfigurando DMA
            uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                                   UDMA_MODE_PINGPONG,
                                   (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                                   ((uint16_t*)adcBuffer1 + 1), ADC_BUFFER_SIZE);
            uDMAChannelEnable(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT);

            // Habilitando DMA para envido dos dados para o PWM

            uDMAChannelTransferSet(UDMA_CHANNEL_TMR1B | UDMA_PRI_SELECT,
                                   UDMA_MODE_PINGPONG,
                                   &outputBuffer1,
                                   (void *)(TIMER1_BASE + TIMER_O_TAMATCHR),
                                   ADC_BUFFER_SIZE);

            uDMAChannelEnable(UDMA_CHANNEL_TMR1B | UDMA_PRI_SELECT);

        }   // Fim da verificação do buffer 1

        if(adcBufStatus[1] == INPUT_FULL){
            // Processando dados

            for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++)
                adcBufferF2[i] = (float32_t) adcBuffer2[i];

            for(uint32_t i = 0; i < numBlocks; i++){
                arm_fir_f32(&S,
                            adcBufferF2 + (i * blockSize),
                            filteredBuffer2 + (i * blockSize), blockSize);
            }

            for(uint32_t i = 0; i < ADC_BUFFER_SIZE; i++){
                adcBuffer2[i] = 0;
                adcData = filteredBuffer2[i] * (float32_t)(PWMLoad/4095);
                outputBuffer2[i] = (uint32_t)(adcData);
            }

            adcBufStatus[1] = INPUT_EMPTY;

            // Reconfigurando DMA
            uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                                   UDMA_MODE_PINGPONG,
                                   (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                                   ((uint16_t*)adcBuffer2 + 1), ADC_BUFFER_SIZE);
            uDMAChannelEnable(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);

            // Habilitando DMA para envido dos dados para o PWM
            uDMAChannelTransferSet(UDMA_CHANNEL_TMR1B | UDMA_ALT_SELECT,
                                   UDMA_MODE_PINGPONG,
                                   &outputBuffer2,
                                   (void *)(TIMER1_BASE + TIMER_O_TAMATCHR),
                                   ADC_BUFFER_SIZE);

            uDMAChannelEnable(UDMA_CHANNEL_TMR1B | UDMA_ALT_SELECT);

        }   // Fim da verificação do buffer 2

    } // Fim do loop infinito
}

void timer0Init(){
    ENABLE_PERIPH(SYSCTL_PERIPH_TIMER0);
    // Timer periódico
    TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);

    // Configurando período
    PWMLoad = sysClkFreq/ADC_SAMPLING_FREQ - 1;
    TimerLoadSet(TIMER0_BASE, TIMER_A, PWMLoad);

    //Iniciando timer
    TimerEnable(TIMER0_BASE, TIMER_A);  // Tirar daqui depois?
}

void timer1Init(){
    ENABLE_PERIPH(SYSCTL_PERIPH_TIMER1);
    ENABLE_PERIPH(SYSCTL_PERIPH_GPIOD);

    //Configurando saída PWM
    GPIOPinConfigure(GPIO_PD2_T1CCP0);
    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_2);

    // Timer A no modo PWM e Timer B no modo periódico
    TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM |
                   TIMER_CFG_B_PERIODIC);

    // Configurando período dos dois timers
    PWMLoad = sysClkFreq/ADC_SAMPLING_FREQ - 1;
    TimerLoadSet(TIMER1_BASE, TIMER_BOTH, PWMLoad);

    // Invertendo timer
    TimerControlLevel(TIMER1_BASE, TIMER_A, true);

    // Configurando duty cycle inicial do timer A
    uint32_t pwm_match = PWM_MATCH_GET(50);
    TimerMatchSet(TIMER1_BASE, TIMER_A, pwm_match);

    //Iniciando timer
    TimerEnable(TIMER1_BASE, TIMER_BOTH);
}

void adc0Init(){
    ENABLE_PERIPH(SYSCTL_PERIPH_GPIOE);
    ENABLE_PERIPH(SYSCTL_PERIPH_ADC0);

    // Configurando pino 3 do GPIOE como ADC (Input 0)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);

    // Configurando sequência 0 para funcionar com o timer como trigger
    ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_TIMER, 0);
    TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

    // Configurando sequência
    ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0|ADC_CTL_IE|
                             ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 0);
}

void udmaInit(){
    ENABLE_PERIPH(SYSCTL_PERIPH_UDMA);

    // Habilitando DMA
    uDMAEnable();           // Tirar daqui?
    // Definindo tabela de controle
    uDMAControlBaseSet(pui8ControlTable);
}

void adc0uDMAConfig(){
    // Desabilitando atributos do canal DMA do ADC0
    uDMAChannelAttributeDisable(UDMA_CHANNEL_ADC0,
                                UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY |
                                UDMA_ATTR_REQMASK);

    // Configurando tabelas de controle
    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT, UDMA_SIZE_16 |
                          UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1);

    uDMAChannelControlSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT, UDMA_SIZE_16 |
                          UDMA_SRC_INC_NONE | UDMA_DST_INC_32 | UDMA_ARB_1);

    // Configurando parâmetros de transferência
    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT,
                           UDMA_MODE_PINGPONG,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                           ((uint16_t*)adcBuffer1 + 1), ADC_BUFFER_SIZE);

    uDMAChannelTransferSet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT,
                           UDMA_MODE_PINGPONG,
                           (void *)(ADC0_BASE + ADC_O_SSFIFO0),
                           ((uint16_t*)adcBuffer2 + 1), ADC_BUFFER_SIZE);

    // Habilitando canal para usar apenas burst
    uDMAChannelAttributeEnable(UDMA_CHANNEL_ADC0, UDMA_ATTR_USEBURST);

    // Habilitando o canal
    uDMAChannelEnable(UDMA_CHANNEL_ADC0);

    // Habilitando DMA para sequência 0 do ADC0
    ADCSequenceDMAEnable(ADC0_BASE, 0);

    // Habilitando interrupção do ADC0 sequência 0
    ADCIntEnable(ADC0_BASE, 0);
    IntEnable(INT_ADC0SS0);
}

void timer1DMAConfig(){
    // Desabilitando atributos do canal DMA do Timer 1
    uDMAChannelAttributeDisable(UDMA_CHANNEL_TMR1B,
                                UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY |
                                UDMA_ATTR_REQMASK);

    // Configurando tabelas de controle
    uDMAChannelControlSet(UDMA_CHANNEL_TMR1B | UDMA_PRI_SELECT, UDMA_SIZE_32 |
                          UDMA_SRC_INC_32 | UDMA_DST_INC_NONE | UDMA_ARB_1);

    uDMAChannelControlSet(UDMA_CHANNEL_TMR1B | UDMA_ALT_SELECT, UDMA_SIZE_32 |
                          UDMA_SRC_INC_32 | UDMA_DST_INC_NONE | UDMA_ARB_1);

    // Configurando parâmetros de transferência
    uDMAChannelTransferSet(UDMA_CHANNEL_TMR1B | UDMA_PRI_SELECT,
                           UDMA_MODE_PINGPONG,
                           &outputBuffer1,
                           (void *)(TIMER1_BASE + TIMER_O_TAMATCHR),
                           ADC_BUFFER_SIZE);

    uDMAChannelTransferSet(UDMA_CHANNEL_TMR1B | UDMA_ALT_SELECT,
                           UDMA_MODE_PINGPONG,
                           &outputBuffer2,
                           (void *)(TIMER1_BASE + TIMER_O_TAMATCHR),
                           ADC_BUFFER_SIZE);

    // Habilitando canal para usar apenas burst
    uDMAChannelAttributeEnable(UDMA_CHANNEL_TMR1B, UDMA_ATTR_USEBURST);

    // Habilitando evento DMA
    TimerDMAEventSet(TIMER1_BASE, TIMER_DMA_TIMEOUT_B);
}

float filter(uint32_t next_sample){
    static uint32_t filter_buffer[FILTER_ORDER + 1];
    static uint32_t counter = 0;

    float output = 0;

    uint32_t filter_index = counter;
    for(uint8_t i = 0; i < FILTER_ORDER + 1; i++){
        output += filter_coefs[i]*filter_buffer[filter_index];

        if(filter_index == 0)
            filter_index = FILTER_ORDER + 1;

        filter_index--;
    }
    counter++;
    counter %= (FILTER_ORDER + 1);

    return output;
}

void adc0s0IntHandler(){
    uint32_t status = ADCIntStatus(ADC0_BASE, 0, true);
    ADCIntClear(ADC0_BASE, 0);

    uint32_t dmaMode;   // Modo do canal DMA

    // Verificando se o canal primário está parado
    dmaMode = uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_PRI_SELECT);
    if(dmaMode == UDMA_MODE_STOP && adcBufStatus[0] == INPUT_FILLING){
        adcBufStatus[0] = INPUT_FULL;
        adcBufStatus[1] = INPUT_FILLING;
    }

    // Verificando se o canal alternativo está parado
    dmaMode = uDMAChannelModeGet(UDMA_CHANNEL_ADC0 | UDMA_ALT_SELECT);
    if(dmaMode == UDMA_MODE_STOP && adcBufStatus[1] == INPUT_FILLING){
        adcBufStatus[0] = INPUT_FILLING;
        adcBufStatus[1] = INPUT_FULL;
    }
}

void timer1BIntHandler(){
    uint32_t status = TimerIntStatus(TIMER1_BASE, true);
    TimerIntClear(TIMER1_BASE, status);
}
