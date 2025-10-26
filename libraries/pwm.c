/*
 * pwm.c
 *
 *  Created on: Oct 7, 2025
 *      Author: clara
 */


#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "pwm.h"
#include "../drivers/gpio.h"

void initPwm()
{
    // Enable clocks
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    SYSCTL_RCGCGPIO_R |= (SYSCTL_RCGCGPIO_R2);
    _delay_cycles(3);



//    GPIO_PORTC_DEN_R |= (32 | 16);
//    GPIO_PORTC_AFSEL_R |= (32 | 16);
//    GPIO_PORTB_DEN_R |= (64 | 128);
//    GPIO_PORTB_AFSEL_R |= (64 | 128);

    GPIO_PORTB_DEN_R |= (64);
    GPIO_PORTB_AFSEL_R |= (64);

//    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC5_M | GPIO_PCTL_PC4_M);
//    GPIO_PORTC_PCTL_R |= (GPIO_PCTL_PC5_M0PWM7 | GPIO_PCTL_PC4_M0PWM6);

//    GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB6_M | GPIO_PCTL_PB7_M);
//    GPIO_PORTB_PCTL_R |= (GPIO_PCTL_PB6_M0PWM0 | GPIO_PCTL_PB7_M0PWM1);

    GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB6_M);
    GPIO_PORTB_PCTL_R |= (GPIO_PCTL_PB6_M0PWM0);


    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0; //Reset pwm0
    SYSCTL_SRPWM_R = 0; // set reset register to 0

    PWM0_0_CTL_R = 0;
//    PWM0_3_CTL_R = 0;

    // PWM0_3_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
    // PWM0_3_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    // PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
    PWM0_0_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    PWM0_0_LOAD_R = 800000; // 50 hz period
    //PWM0_3_LOAD_R = 1024;


    //PWM0_0_CMPB_R = 0;
    PWM0_0_CMPA_R = 0;
    // PWM0_3_CMPB_R = 0;
    // PWM0_3_CMPA_R = 0;

    //PWM0_3_CTL_R = PWM_0_CTL_ENABLE;
    PWM0_0_CTL_R = PWM_0_CTL_ENABLE;

    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN //| PWM_ENABLE_PWM1EN | PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN);
}

// 64000 for slow

// 60000 for off

void servoSlow(){ // Forward a constant speed
    PWM0_0_CMPA_R = 64000;
}


void servoStop(){ // Zero All Motor Speeds
    PWM0_0_CMPA_R = 60000;
}


