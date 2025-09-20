// Tasks
// D McComas

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "../dependencies/tm4c123gh6pm.h"
#include "../dependencies/wait.h"

#include "../kernel/mm.h"
#include "../kernel/kernel.h"
#include "tasks.h"

#include "../drivers/gpio.h"
#include "../drivers/pain.h"
#include "../drivers/gpio.h"
#include "../drivers/uart0.h"
#include "../drivers/clock.h"



#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTE,0 // off-board red LED
#define ORANGE_LED PORTA,2 // off-board orange LED
#define YELLOW_LED PORTA,3 // off-board yellow LED
#define GREEN_LED  PORTA,4 // off-board green LED


// PB1 - Stop Flash 4 Hz, Uncooperative

// PB2 - Set LengthyFn Prio 4

// PB3 - Errant

// PB4 - LEDs

// PB5 - One Shot

// PB6 - Restart Flash 4 Hz

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    initSystemClockTo40Mhz();
    // Setup LEDs and pushbuttons
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5 | SYSCTL_RCGCGPIO_R4 | SYSCTL_RCGCGPIO_R0;
    _delay_cycles(3);
    initUart0();
    initPain();

    SYSCTL_RCGCTIMER_R |= (SYSCTL_RCGCTIMER_R1 | SYSCTL_RCGCTIMER_R5);

    setUart0BaudRate(115200,40000000); // Set baud rate to project spec
    // Configure LED pins
//    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK;  // bits 1 and 3 are outputs
//    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
//    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | RED_LED_MASK;  // enable LEDs



//    TIMER1_CTL_R &= ~(TIMER_CTL_TAEN);                 // turn-off timer before reconfiguring
//    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
//    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;        // configure for periodic mode (count down)
//    TIMER1_TAILR_R = 40000000;                         // set load value to 10e5 for 25 ms interrupt rate
//    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
//
//    TIMER1_IMR_R |= TIMER_IMR_TATOIM;                  // turn-on debounce interrupt


    //NVIC_EN0_R = 1 << (INT_TIMER1A-16);              // turn-on interrupt 37 (TIMER1A)


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

void timer1Isr(){
    //GREEN_OB_LED ^= 1;
    TIMER1_ICR_R |= TIMER_ICR_TATOCINT;
}


uint8_t readPbs(void)
{
    uint8_t result = ((getPortValue(PORTD)) >> 2) | ((getPortValue(PORTC)) >> 4);
    return result;
}


// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 0); // Reversed these to fix the LED being stuck on, doesn't change how the code works across systems :)
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 1);
        yield();
    }
}


void idle2(void)
{
    while(true)
    {
        setPinValue(BLUE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(BLUE_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
       // setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = mallocUnprivFromHeap((void *)(5000 * sizeof(uint8_t)));
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((~buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((~buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((~buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((~buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((~buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}



void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() != 0x3F)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 0x37)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 0x1F)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}

void restartShell(void)
{
    while(true)
    {
        saveShell();
        sleep(5000);
        yield();
    }
}
