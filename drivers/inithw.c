// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
#include "../libraries/tm4c123gh6pm.h"
#include "../libraries/wait.h"

#include "../kernel/mm.h"
#include "../kernel/kernel.h"

#include "../drivers/gpio.h"
#include "../drivers/pain.h"
#include "../drivers/gpio.h"
#include "../drivers/uart0.h"
#include "../drivers/clock.h"

#include "../libraries/pwm.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTE,0 // off-board red LED
#define ORANGE_LED PORTA,2 // off-board orange LED
#define YELLOW_LED PORTA,3 // off-board yellow LED
#define GREEN_LED  PORTA,4 // off-board green LED




void initHw(void)
{
    initSystemClockTo40Mhz();
    // Setup LEDs and pushbuttons
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5 | SYSCTL_RCGCGPIO_R4 | SYSCTL_RCGCGPIO_R0;
    _delay_cycles(3);
    initUart0();
    initPain();
    initPwm();

    SYSCTL_RCGCTIMER_R |= (SYSCTL_RCGCTIMER_R1 | SYSCTL_RCGCTIMER_R5);

    setUart0BaudRate(115200,40000000); // Set baud rate to project spec


    NVIC_CFG_CTRL_R |= NVIC_CFG_CTRL_DIV0;


    NVIC_SYS_HND_CTRL_R |= (NVIC_SYS_HND_CTRL_USAGE | NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_MEM);

    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN;
    NVIC_ST_RELOAD_R = 39999;
    NVIC_ST_CURRENT_R = 0;



    enablePort(PORTD);
    enablePort(PORTC);
    enablePort(PORTB);

    setPinCommitControl(PORTD, 7);


    selectPinDigitalInput(PORTD, 7);
    selectPinDigitalInput(PORTD, 7);
    selectPinDigitalInput(PORTD, 6);
    //selectPinDigitalInput(PORTC, 7);
    //selectPinDigitalInput(PORTB, 6);
    selectPinDigitalInput(PORTC, 6);
    selectPinDigitalInput(PORTC, 5);
    selectPinDigitalInput(PORTC, 4);

    selectPinPushPullOutput(PORTA, 2);
    selectPinPushPullOutput(PORTA, 3);
    selectPinPushPullOutput(PORTA, 4);
    selectPinPushPullOutput(PORTE, 0);
    selectPinPushPullOutput(PORTF, 2);

    enablePinPullup(PORTD, 7);
    enablePinPullup(PORTD, 6);
    enablePinPullup(PORTC, 7);
    enablePinPullup(PORTC, 6);
    enablePinPullup(PORTC, 5);
    enablePinPullup(PORTC, 4);

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);

}
