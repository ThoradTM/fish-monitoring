/*
 * pain.c
 *
 *  Created on: Mar 24, 2024
 *      Author: clara
 */
#include "../libraries/tm4c123gh6pm.h"
#include "pain.h"


void initPain(){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    GPIO_PORTF_DIR_R |= GREEN_LED_MASK;
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK;

    GREEN_OB_LED = 0;
}




