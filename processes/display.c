#include "display.h"               
#include "../libraries/graphics_lcd.h"
#include "../kernel/shm.h"
#include "../kernel/kernel.h"
#include "../kernel/servicecalls.h"
#include "../drivers/uart0.h"
#include "../libraries/pwm.h"
#include "../libraries/wait.h"


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
	setGraphicsLcdTextPosition(lcdHandler, 20, 2);                 // Set text position
	putsGraphicsLcd(lcdHandler, "Task Running");                   // Print "Task Running" at the set position
	setGraphicsLcdTextPosition(lcdHandler, 20, 3);                 // Set text position
	putsGraphicsLcd(lcdHandler, "Temperature: ");
	
	lock(resource);

	putiGraphicsLcd(lcdHandler, shmHandle->temperature);
	unlock(resource);

	sleep(100);
}


void settingsTask(DisplayContext * lcdHandler)
{

}

void screensaverTask(DisplayContext * lcdHandler)
{

}

void consumerStateMachine(states state, shm * shmHandle, DisplayContext * lcdHandler)
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
			screensaverTask(lcdHandler);
			break;
	}
}

states changeState(states state)
{
	return DISPLAY;
}

void doScheduledTask()
{
	sleep(1000);
	servoSlow();
	sleep(173);
	servoStop();
}

void consumerLoop()
{
	DisplayContext myDisplay;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * shmHandle = getShmHandle();

	states state = DISPLAY;
	
	while(1)
	{
		sleep(1000);
		state = changeState(state);
		consumerStateMachine(state, shmHandle, lcdHandler);

		// Placeholder to check if its time to do a task
		doScheduledTask();
	}
}
