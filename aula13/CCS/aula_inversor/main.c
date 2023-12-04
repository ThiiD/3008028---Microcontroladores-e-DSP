/*****************************************************************************
Exemplo mostrando o controle de um inversor Full-Bridge utilizando
modulação SPWM bipolar e controlador PI modificado

Autor: prof. Guilherme Márcio Soares
Data: 28/11/2023
*****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/fpu.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "utils/uart_utils.h"

#define REF_FREQ 60             //Frequência da modulante

#define NUM_SEC 10              //Número de segundos para troca de referência
#define PWM_FREQUENCY 20000     //Frequência do PWM
#define PWM_PERIOD 5e-5         //Período do PWM

//Coeficientes do sensor
#define COEFF_A 0.007712        //Ganho do sensor
#define COEFF_B 1312            //Offset do sensor

//Parâmetros do controlador
#define B0 0.036142
#define B1 0.000592
#define B2 -0.035549
#define A1 -1.521886
#define A2 0.521886
#define UL 0.5
#define LL -0.5

uint32_t g_ui32SysClock;
volatile float amplitude;
volatile uint16_t pwm_load=0;
volatile int16_t duty=0;
volatile float y[3] = {0};
volatile float x[3] = {0};
volatile uint16_t sample;
volatile int change=0;
volatile float refs[3] = {1,1.5,2};

void PWM0Gen1IntHandler(void)
{

    MAP_PWMGenIntClear(PWM0_BASE, PWM_GEN_1, PWM_INT_CNT_LOAD);
    static uint16_t ti=0;
    static float ref;
    static int index_ref=0;

    if(change>(NUM_SEC*PWM_FREQUENCY))
    {
        change = 0;
        index_ref=(index_ref+1)%3;
    }
    change++;

    ref = refs[index_ref]*sinf(2*M_PI*REF_FREQ*PWM_PERIOD*ti);
    ti = (++ti)%(PWM_FREQUENCY);

    while(!ADCIntStatus(ADC0_BASE, 3, false));
    ADCIntClear(ADC0_BASE, 3);
    ADCSequenceDataGet(ADC0_BASE, 3, (uint32_t*)&sample);

    x[2] = x[1]; x[1] = x[0]; x[0] = ref - (COEFF_A*(sample-COEFF_B));
    y[2] = y[1]; y[1] = y[0];
    y[0] = B0*x[0] + B1*x[1] + B2*x[2] - A1*y[1] - A2*y[2];

    y[0] = (y[0] > UL) ? UL : y[0];
    y[0] = (y[0] < LL) ? LL : y[0];

    duty = (int16_t)(pwm_load*(y[0]+0.5));
    duty = (uint16_t)((duty > (0.9*pwm_load)) ? 0.9*pwm_load : duty);
    duty = (uint16_t)((duty < (0.05*pwm_load)) ? 0.1*pwm_load : duty);

    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, duty);
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, duty);

    PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT);
}

void configUART(uint32_t sys_clk_freq)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(
            UART0_BASE, sys_clk_freq, 115200,
            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

void config_adc()
{
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    ADCReferenceSet(ADC0_BASE, ADC_REF_INT);
    IntDisable(INT_ADC0SS0);
    ADCIntDisable(ADC0_BASE, 3);
    ADCSequenceDisable(ADC0_BASE, 3);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PWM1, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH3 | ADC_CTL_END | ADC_CTL_IE);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);

}
void config_pwm(uint32_t sys_clock)
{
    MAP_GPIOPinConfigure(GPIO_PF1_M0PWM1);
    MAP_GPIOPinConfigure(GPIO_PF2_M0PWM2);
    MAP_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    MAP_GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);

    MAP_PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1);

    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
                        PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC);
    MAP_PWMGenConfigure(PWM0_BASE, PWM_GEN_1,
                           PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_SYNC);


    PWMSyncTimeBase(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT);


    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, (sys_clock/ PWM_FREQUENCY));
    MAP_PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, (sys_clock/ PWM_FREQUENCY));

    PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_1, PWM_INT_CNT_ZERO | PWM_TR_CNT_LOAD);
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1,0);
    MAP_PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,0);

    MAP_PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
    MAP_PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);

    MAP_PWMOutputInvert(PWM0_BASE, PWM_OUT_2_BIT, true);

    MAP_PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    MAP_PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    PWMSyncUpdate(PWM0_BASE, PWM_GEN_0_BIT | PWM_GEN_1_BIT);

    pwm_load = MAP_PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1);
}


int
main(void)
{

    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_FPUEnable();
    MAP_FPUStackingEnable();

    config_adc();
    config_pwm(g_ui32SysClock);
    configUART(g_ui32SysClock);
    MAP_IntMasterEnable();

    MAP_PWMIntEnable(PWM0_BASE, PWM_INT_GEN_1);
    MAP_PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_1, PWM_INT_CNT_LOAD);
    MAP_IntEnable(INT_PWM0_1);

    IntMasterEnable();

    while(1)
    { }
}
