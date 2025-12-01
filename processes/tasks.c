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
#include "../libraries/tm4c123gh6pm.h"
#include "../libraries/wait.h"

#include "../kernel/mm.h"
#include "../kernel/kernel.h"
#include "../kernel/servicecalls.h"
#include "../kernel/shm.h"
#include "tasks.h"

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


// PB1 - Stop Flash 4 Hz, Uncooperative

// PB2 - Set LengthyFn Prio 4

// PB3 - Errant

// PB4 - LEDs

// PB5 - One Shot

// PB6 - Restart Flash 4 Hz

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

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

// We've reached this point, its the only way.
void shm_task()
{
    while(1);
}

void idle(void)
{
    while(true)
    {
//        setPinValue(ORANGE_LED, 0); // Reversed these to fix the LED being stuck on, doesn't change how the code works across systems :)
//        waitMicrosecond(1000);
//        setPinValue(ORANGE_LED, 1);

        setPinValue(GREEN_LED, 1);
        waitMicrosecond(1000);
        setPinValue(GREEN_LED, 0);

        yield();
    }
}

void shmTestWriter(void)
{
    shmPerms();
    shm * test = getShmHandle();

    while(true)
    {
        lock(resource);
        test->shared++;
        unlock(resource);
        sleep(1000);
    }
}

void shmTestReader(void)
{
    shmPerms();
    shm * test = getShmHandle();

    while(true)
    {
        lock(resource);
        putiUart0((test->shared));
        unlock(resource);
        putcUart0('\n');
        sleep(1000);
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

void doFeedTask()
{
    shmPerms();
    shm * shmHandle = getShmHandle();

	int i;
	while(1)
	{
        sleep(1000);
        if(shmHandle->servoFlag)
        {
            for(i = 0; i < shmHandle->feedingAmounts; i++)
            {
                PWM0_0_CMPA_R = 4000;
                sleep(180);
                PWM0_0_CMPA_R = 3750;
                sleep(1000);
                shmHandle->servoFlag = 0;
            }
        }
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

#define PUSH_BUTTON 4

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void readKeys(void)
{
    shm * shmHandle = getShmHandle();
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = getPinValue(PORTF, PUSH_BUTTON);
            yield();
        }
        post(keyPressed);
        if (buttons)
        {
            shmHandle->presses++;
        }
        yield();
    }
}

void debounce(void)
{
    shmPerms();
    shm * shmHandle = getShmHandle();
    uint8_t count;
    while(true)
    {
        while(getPinValue(PORTF, PUSH_BUTTON) != 0);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (getPinValue(PORTF, PUSH_BUTTON) != 0)
                count--;
            else
                count = 10;
        }
        shmHandle->presses++;
    }
}

void debounce2(void)
{
    shmPerms();
    shm * shmHandle = getShmHandle();
    uint8_t count;
    while(true)
    {
        while(getPinValue(PORTF, 0) != 0);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (getPinValue(PORTF, 0) != 0)
                count--;
            else
                count = 10;
        }
        shmHandle->presses2++;
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
