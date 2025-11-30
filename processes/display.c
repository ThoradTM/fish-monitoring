#include "display.h"               
#include "../libraries/graphics_lcd.h"
#include "../kernel/shm.h"
#include "../kernel/kernel.h"
#include "../kernel/servicecalls.h"
#include "../drivers/uart0.h"
#include "../libraries/pwm.h"
#include "../libraries/wait.h"
#include "../libraries/tm4c123gh6pm.h"
#include "tasks.h"
#include <stdint.h>
#include "../drivers/rtc.h"
#include "../drivers/gpio.h"

#define PUSH_BUTTON 4

typedef struct savedVars
{
	int x;
	int page;
}savedVars;


// Stup port to 
void initDisplay(DisplayContext * myDisplay)
{
	waitMicrosecond(10);
	initGraphicsLcd(myDisplay);                   // Initialize the graphics LCD hardware and state
	waitMicrosecond(10);
	clearGraphicsLcd(myDisplay);          		   // Clear the display buffer and the physical screen
	waitMicrosecond(10);
	setGraphicsLcdTextPosition(myDisplay, 0, 0);  // Set the text cursor to the top-left corner
	waitMicrosecond(10);
	putsGraphicsLcd(myDisplay, "Display Ready");  // Print "Display Ready" on the LCD
	waitMicrosecond(10000);
}

typedef enum states
{
	SCREENSAVER,
	DISPLAY,
	TIME,
	SETTINGS
}states;

void displayTask(DisplayContext * lcdHandler, shm * shmHandle)
{
	clearGraphicsLcd(lcdHandler);
	setGraphicsLcdTextPosition(lcdHandler, 0, 0);                 // Set text position
	putsGraphicsLcd(lcdHandler, "Turbidity: ");                   // Print "Task Running" at the set position
	lock(resource);

	putiGraphicsLcd(lcdHandler, shmHandle->turbidity);
	unlock(resource);
	setGraphicsLcdTextPosition(lcdHandler, 0, 1);                 // Set text position
	putsGraphicsLcd(lcdHandler, "Temperature: ");
	
	lock(resource);

	putiGraphicsLcd(lcdHandler, shmHandle->temperature);
	unlock(resource);
}

void timeTask(DisplayContext * lcdHandler)
{
	RTC_DateTime time;
	RTC_GetDateTime(&time);

	clearGraphicsLcd(lcdHandler);
	setGraphicsLcdTextPosition(lcdHandler, 10, 0);                 // Set text position
	putsGraphicsLcd(lcdHandler, "Time: ");                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 10, 1);                 // Set text position
	putiGraphicsLcd(lcdHandler, time.seconds);                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 10, 2);                 // Set text position
	putiGraphicsLcd(lcdHandler, time.minutes);                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 10, 3);                 // Set text position
	putiGraphicsLcd(lcdHandler, time.hours);                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 10, 4);                 // Set text position
	putiGraphicsLcd(lcdHandler, time.day);                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 10, 5);                 // Set text position
	putiGraphicsLcd(lcdHandler, time.month);                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 10, 6);                 // Set text position
	putiGraphicsLcd(lcdHandler, time.year);                   // Print "Task Running" at the set position
}

void settingsTask(DisplayContext * lcdHandler)
{

}

void screensaverTask(DisplayContext * lcdHandler, savedVars * passthrough)
{
	if(passthrough->page > 5)
		passthrough->page = 0;
	if(passthrough->x > 100)
		passthrough->x = 0;
	passthrough->x += 5;
	passthrough->page++;
	clearGraphicsLcd(lcdHandler);
	setGraphicsLcdTextPosition(lcdHandler, passthrough->x, passthrough->page);                 // Set text position
	putFishGraphicsLcd(lcdHandler);                   // Print "Task Running" at the set position
}

void consumerStateMachine(states state, shm * shmHandle, DisplayContext * lcdHandler, savedVars * passthrough)
{
	switch(state)
	{
		case SCREENSAVER: 
			screensaverTask(lcdHandler, passthrough);
			break;
		
		case DISPLAY:
			displayTask(lcdHandler,shmHandle);
			break;
		
		case TIME:
			timeTask(lcdHandler);
			break;

		case SETTINGS:
			settingsTask(lcdHandler);
			break;
	}
}


// To do:
// Set hourly feeding interval

void doScheduledTask()
{
	// PWM0_0_CMPA_R = 4000;
	//servoSlow();
	//TIMER5_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
}

void turbidityAlarm(shm * shmHandle)
{
	if(shmHandle->turbidity < 2500)
	{
		if(getPinValue(PORTC, 6))
		{
			setPinValue(PORTC, 6, 0);
		}
		else 
		{
			setPinValue(PORTC, 6, 1);
		}
	}
	else 
	{
		if(getPinValue(PORTC, 6))
			setPinValue(PORTC, 6, 0);
		
	}
}

void consumerLoop()
{
	DisplayContext myDisplay;
	savedVars passthrough;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * shmHandle = getShmHandle();

	passthrough.x = 0;
	passthrough.page = 0;

	states state = SCREENSAVER;





	while(1)
	{
		// sleep(1000);
		//uint8_t buttons;

        //buttons = getPinValue(PORTF, PUSH_BUTTON);
		// if(shmHandle->presses > 0)
		// {
		// 	//sleep(1000);
		// 	state++;
		// 	if(state > 3)
		// 		state = SCREENSAVER;
		// 	shmHandle->presses--;
		// }
		// sleep(1000);
		
        //buttons = getPinValue(PORTF, PUSH_BUTTON);
		sleep(1000);
		if(shmHandle->presses > 0)
		{
			state++;
			if(state > 3)
				state = SCREENSAVER;
			shmHandle->presses--;
		}
		consumerStateMachine(state, shmHandle, lcdHandler, &passthrough);
				// Placeholder to check if its time to do a task
		turbidityAlarm(shmHandle);
		//doScheduledTask();
	}
}
