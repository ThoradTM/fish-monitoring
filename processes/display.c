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
	DISPLAY,
	SETTINGS,
	SCREENSAVER
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

	sleep(100);
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
	sleep(100);
}

void consumerStateMachine(states state, shm * shmHandle, DisplayContext * lcdHandler, savedVars * passthrough)
{
	switch(state)
	{
		case DISPLAY: 
			displayTask(lcdHandler, shmHandle);
			break;
		
		case SETTINGS:
			settingsTask(lcdHandler);
			break;
		
		case SCREENSAVER:
			screensaverTask(lcdHandler, passthrough);
			break;
	}
}

states changeState(states state)
{
	return SCREENSAVER;
}

void doScheduledTask()
{
	//servoSlow();
	//TIMER5_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
}

void consumerLoop()
{
	DisplayContext myDisplay;
	savedVars passthrough;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * shmHandle = getShmHandle();

	states state = DISPLAY;

	// uint8_t buttons;

	// wait(keyPressed);
	// buttons = 0;
	// while (buttons == 0)
	// {
	// 	buttons = readPbs();
	// 	yield();
	// }
	//post(keyPressed);
	
	while(1)
	{
		sleep(1000);
		state = changeState(state);
		consumerStateMachine(state, shmHandle, lcdHandler, &passthrough);

		// Placeholder to check if its time to do a task
		doScheduledTask();
	}
}
