/*
 * turbidity.c
 *
 *  Created on: Sep 19, 2025
 *      Author: clara
 */
#include "turbidity.h"
#include "adc.h"                     // Include ADC driver for sensor reading
#include "gpio.h"                    // Include GPIO driver if needed   

void initTurbidity()
{
    // Initialize ADC for turbidity sensor
    // Configure GPIO pins if necessary
    // Set up any required timers or interrupts
}

void turbidityTask()
{
    while (1)
    {
        // Read turbidity sensor value using ADC
        // Process the sensor data (e.g., convert to NTU)
        // Optionally, send the data to another task or log it

        for (volatile int i = 0; i < 1000000; i++); // Simple delay to control task frequency
    }
}

