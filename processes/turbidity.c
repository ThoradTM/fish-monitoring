/*
 * turbidity.c
 *
 *  Created on: Sep 19, 2025
 *      Author: Dylan Nguyen
 */
#include <stdint.h>
#include <stdbool.h>
#include "../kernel/kernel.h"     // sleep()
#include "../dependencies/wait.h" // waitMicrosecond()
#include "../drivers/uart0.h"     // putsUart0()
#include "../drivers/gpio.h"      // enablePort(), selectPinAnalogInput()
#include "../dependencies/tm4c123gh6pm.h"

#ifndef putuart
#define putuart putsUart0
#endif

// Initialization code for turbidity sensor (DFRobot SEN0189) on PE0 (AIN3)
void initTurbidity(void)
{
    // Use GPIO helper to init Port E and configure PE0 as analog input
    enablePort(PORTE);
    selectPinAnalogInput(PORTE, 0); // PE0 -> AIN3

    // Enable ADC0 clock and wait ready
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0; // ADC0 clock
    while ((SYSCTL_PRADC_R & SYSCTL_PRADC_R0) == 0);

    // ADC0 setup for single sample on SS3, software trigger, AIN3 (PE0)
    ADC0_PC_R = 0x01;      // 125 ksps
    ADC0_SSPRI_R = 0x3210; // SS3 highest priority

    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3; // disable SS3 during config
    ADC0_EMUX_R = (ADC0_EMUX_R & ~ADC_EMUX_EM3_M) | ADC_EMUX_EM3_PROCESSOR; // Select software (processor) trigger for SS3
    ADC0_SSMUX3_R = 3;                                // AIN3 -> PE0
    ADC0_SSCTL3_R = ADC_SSCTL3_END0 | ADC_SSCTL3_IE0; // single sample, EoS
    ADC0_IM_R &= ~ADC_IM_MASK3;                       // no interrupt (polling)
    ADC0_ISC_R = ADC_ISC_IN3;                         // clear any prior flag
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                  // enable SS3
}

// Read one 12-bit sample from ADC0 SS3 (PE0/AIN3)
uint16_t readTurbidityRaw(void)
{
    ADC0_PSSI_R = ADC_PSSI_SS3;              // start conversion
    while ((ADC0_RIS_R & ADC_RIS_INR3) == 0); // wait complete
    uint16_t result = (uint16_t)(ADC0_SSFIFO3_R & 0x0FFF);
    ADC0_ISC_R = ADC_ISC_IN3; // clear flag
    return result;
}

void turbidityTask(void)
{
    initTurbidity();

    while (true)
    {
        // Debug output
        putsUart0("Analog Value: ");

        // Read raw ADC (unused for now)
        putiUart0((uint32_t)readTurbidityRaw());
        putcUart0("\n");
        // Sleep for 1000 ms
        sleep(1000);
    }
}