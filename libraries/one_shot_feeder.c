/*
 * pir.c
 *
 *  Created on: Apr 19, 2024
 *      Author: clara
 */

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "pain.h"
#include "pir.h"
#define PIR_BB     (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 4*4)))

void initPir(){
    SYSCTL_RCGCGPIO_R |= (SYSCTL_RCGCGPIO_R1);
    SYSCTL_RCGCTIMER_R |= (SYSCTL_RCGCTIMER_R5);


    _delay_cycles(3);

    TIMER5_CTL_R &= ~(TIMER_CTL_TAEN);                 // turn-off timer before reconfiguring
    TIMER5_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER5_TAMR_R = TIMER_TAMR_TAMR_PERIOD;        // configure for periodic mode (count down)
    TIMER5_TAILR_R = 6920000;                         // set load value for 173ms interrupt rate
                      // turn-on timer

    // Configure keyboard
    // Columns 0-3 with open-drain outpus connected to PB0, PB1, PB4, PA6
    // Rows 0-3 with pull-ups connected to PE1, PE2, PE3, PA7
    GPIO_PORTB_DEN_R |= PIR;

    GPIO_PORTB_DIR_R &= ~PIR;

    GPIO_PORTB_IS_R &= ~(PIR); // Enable edge interrupts

    GPIO_PORTB_IBE_R |= PIR; // double edge interrupt

    //GPIO_PORTB_IEV_R |= PIR; //posedge interrupt

    GPIO_PORTB_ICR_R |= PIR; // Clear InterruptsGPIO_PORTC_IM_R |= W1;

    //GPIO_PORTB_IM_R |= PIR;

//    TIMER5_IMR_R |= TIMER_IMR_TATOIM;                  // turn-on debounce interrupt
    TIMER5_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer

    NVIC_EN0_R = 1 << (INT_GPIOB-16);                // turn-on interrupt 19 (GPIOD)
    NVIC_EN2_R = 1 << (INT_TIMER5A-16-64);
    initPain();
}

void pirIsr(){
    GPIO_PORTB_ICR_R |= PIR;

//    TIMER5_ICR_R |= TIMER_ICR_TATOCINT;
//    TIMER5_IMR_R |= TIMER_IMR_TATOIM;                  // turn-on debounce interrupt
//    TIMER5_TAV_R = TIMER5_TAILR_R;
//    TIMER5_CTL_R |= TIMER_CTL_TAEN;
}

void timer5Isr(){
    servoStop(); // Stop servo rotation
    TIMER5_ICR_R |= TIMER_ICR_TATOCINT;
}