/*
 * pwm.c
 *
 *  Created on: Oct 7, 2025
 *      Author: clara
 */


#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "pwm.h"

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

    GPIO_PORTB_DEN_R |= (128);
    GPIO_PORTB_AFSEL_R |= (128);

//    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC5_M | GPIO_PCTL_PC4_M);
//    GPIO_PORTC_PCTL_R |= (GPIO_PCTL_PC5_M0PWM7 | GPIO_PCTL_PC4_M0PWM6);

//    GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB6_M | GPIO_PCTL_PB7_M);
//    GPIO_PORTB_PCTL_R |= (GPIO_PCTL_PB6_M0PWM0 | GPIO_PCTL_PB7_M0PWM1);

    GPIO_PORTB_PCTL_R &= ~(GPIO_PCTL_PB7_M);
    GPIO_PORTB_PCTL_R |= (GPIO_PCTL_PB7_M0PWM1);


    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0; //Reset pwm0
    SYSCTL_SRPWM_R = 0; // set reset register to 0

    PWM0_0_CTL_R = 0;
    PWM0_3_CTL_R = 0;

    PWM0_3_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
    PWM0_3_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
    PWM0_0_GENA_R = PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO;

    PWM0_0_LOAD_R = 1024;
    PWM0_3_LOAD_R = 1024;


    PWM0_0_CMPB_R = 0;
    PWM0_0_CMPA_R = 0;
    PWM0_3_CMPB_R = 0;
    PWM0_3_CMPA_R = 0;

    PWM0_3_CTL_R = PWM_0_CTL_ENABLE;
    PWM0_0_CTL_R = PWM_0_CTL_ENABLE;

    PWM0_ENABLE_R |= (PWM_ENABLE_PWM0EN | PWM_ENABLE_PWM1EN | PWM_ENABLE_PWM6EN | PWM_ENABLE_PWM7EN);
}

void motorSpeedFw(int ms0){ // Forward a constant speed
    desired_pwm = ms0;
    PWM0_0_CMPA_R = ms0;
    PWM0_3_CMPB_R = ms0 + sf;
}

void motorSpeedBw(int ms0){ // Backwards a constant speed
    desired_pwm = ms0;
    PWM0_0_CMPB_R = ms0;
    PWM0_3_CMPA_R = ms0 + sf;
}

void motorSpeedCcw(int ms0){ // Clockwise
    desired_pwm = ms0;
    PWM0_0_CMPB_R = ms0;
    PWM0_3_CMPB_R = ms0 + sf;
}

void motorSpeedCw(int ms0){ // Counterclockwise
    desired_pwm = ms0;
    PWM0_0_CMPA_R = ms0;
    PWM0_3_CMPA_R = ms0 + sf;
}



void motorSpeedZero(){ // Zero All Motor Speeds
    desired_pwm = 0;
    PWM0_0_CMPB_R = 0;
    PWM0_0_CMPA_R = 0;
    PWM0_3_CMPB_R = 0;
    PWM0_3_CMPA_R = 0;
}


