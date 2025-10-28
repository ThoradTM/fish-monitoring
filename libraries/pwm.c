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

    SYSCTL_RCC_R |= SYSCTL_RCC_PWMDIV_16 | SYSCTL_RCC_USEPWMDIV;
    _delay_cycles(3);

    enablePort(PORTB);

//    GPIO_PORTC_DEN_R |= (32 | 16);
//    GPIO_PORTC_AFSEL_R |= (32 | 16);
   GPIO_PORTB_DEN_R |= (64 | 128);
   GPIO_PORTB_AFSEL_R |= (64 | 128);

//    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC5_M | GPIO_PCTL_PC4_M);
//    GPIO_PORTC_PCTL_R |= (GPIO_PCTL_PC5_M0PWM7 | GPIO_PCTL_PC4_M0PWM6);

    GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB6_M | GPIO_PCTL_PB7_M);
    GPIO_PORTB_PCTL_R |= (GPIO_PCTL_PB6_M0PWM0 | GPIO_PCTL_PB7_M0PWM1);



    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0; //Reset pwm0
    SYSCTL_SRPWM_R = 0; // set reset register to 0

    PWM0_0_CTL_R = 0;
//    PWM0_3_CTL_R = 0;

    // PWM0_3_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
    // PWM0_3_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    //PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
    PWM0_0_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    PWM0_0_LOAD_R = 50000; // 50 hz period
    //PWM0_3_LOAD_R = 799999;


    //PWM0_0_CMPB_R = 400000;
    PWM0_0_CMPA_R = 0;
    // PWM0_3_CMPB_R = 0;
    // PWM0_3_CMPA_R = 0;

    //PWM0_3_CTL_R = PWM_0_CTL_ENABLE;
    PWM0_0_CTL_R = PWM_0_CTL_ENABLE;

    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN; // | PWM_ENABLE_PWM1EN;// | PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN);
}

// 64000 for slow

// 60000 for off

void buzzerOn()
{
    PWM0_0_CMPB_R = 799999;
}

void buzzerOff()
{
    PWM0_0_CMPB_R = 0;
}

void servoSlow(){ // Forward a constant speed
    PWM0_0_CMPA_R = 4000;
    //PWM0_0_CMPA_R = 128000;
}


void servoStop(){ // Zero All Motor Speeds
    PWM0_0_CMPA_R = 3750;
    //PWM0_0_CMPA_R = 120000;
}


