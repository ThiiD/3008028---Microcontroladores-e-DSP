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
#include "utils/uart_utils.h"
#include "arm_math.h"


/* ----------------------------------------------------------------------
 ** Macro Defines
 ** ------------------------------------------------------------------- */

#define N_SAMPLES             320
#define BLOCK_SIZE            32
#define NUM_TAPS              29
#define NUM_TAPS_ARRAY_SIZE   29

extern float32_t input_signal[N_SAMPLES];
static float32_t output_signal[N_SAMPLES];

/* -------------------------------------------------------------------
 * Declare State buffer of size (numTaps + blockSize - 1)
 * ------------------------------------------------------------------- */

static float32_t firStateF32[BLOCK_SIZE + NUM_TAPS - 1];

const float32_t firCoeffs32[NUM_TAPS_ARRAY_SIZE] = {
  -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f, +0.0085302217f, -0.0000000000f, -0.0173976984f,
  -0.0341458607f, -0.0333591565f, +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f, +0.2229246956f,
  +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f, -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f,
  +0.0080754303f, +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f
};


uint32_t blockSize = BLOCK_SIZE;
uint32_t numBlocks = N_SAMPLES / BLOCK_SIZE;


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

int main(void)
{
    uint32_t ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
    SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                                 120000000);
    configUART(ui32SysClkFreq);
    IntMasterEnable();
    static arm_fir_instance_f32 S;
    arm_fir_init_f32(&S, NUM_TAPS, (float32_t*) firCoeffs32, firStateF32, blockSize);

    while(1)
    {
        if (UARTCharsAvail(UART0_BASE))
        {
            UARTCharGet(UART0_BASE);
            uint32_t i;
            for (i = 0; i < numBlocks; i++)
            {
                arm_fir_f32(&S, input_signal + (i * blockSize), output_signal + (i * blockSize),
                            blockSize);
            }
            for(i = 0; i < N_SAMPLES; i++)
            {
                send_float(UART0_BASE, input_signal[i]);
            }
            for(i = 0; i < N_SAMPLES; i++)
            {
                send_float(UART0_BASE, output_signal[i]);
            }
        }

    }


}
