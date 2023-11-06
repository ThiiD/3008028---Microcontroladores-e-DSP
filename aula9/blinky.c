#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"


#define PWM_FREQUENCY 100
#define APP_PI 3.1415926535897932384626433832795f
#define STEPS 256

int main(void)
{
    volatile uint32_t ui32Load; // PWM period
    volatile uint32_t ui32BlueLevel; // PWM duty cycle for blue LED
    volatile uint32_t ui32PWMClock; // PWM clock frequency
    volatile uint32_t ui32SysClkFreq; // Value returned by SysClockFreqSet()
    volatile uint32_t ui32Index; // Counts the calculation loops
    float fAngle; // Value for sine math (radians)

    ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN
    | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3, 0x00);

    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);

    GPIOPinConfigure(GPIO_PG0_M0PWM4);
    GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0);

    ui32PWMClock = ui32SysClkFreq / 64; // 120MHz/64
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1; // 1875000/100

    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, ui32Load);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, ui32Load/2);
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

    ui32Index = 0;
    while(1)
    {
    fAngle = ui32Index * (2.0f * APP_PI/STEPS);
    ui32BlueLevel = (uint32_t) (9370.0f * (1 + sinf(fAngle)));
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, ui32BlueLevel + 1);
    ui32Index++;
    if (ui32Index == (STEPS - 1))
    {
    ui32Index = 0;
    }
    SysCtlDelay(ui32SysClkFreq/(STEPS));
    }
}
