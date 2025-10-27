#include "display.h"               
#include "../libraries/graphics_lcd.h"
#include "../kernel/shm.h"
#include "../kernel/kernel.h"
#include "../kernel/servicecalls.h"
#include "../drivers/uart0.h"
#include "../libraries/pwm.h"



void initDisplay(DisplayContext * myDisplay)
{
	initGraphicsLcd(myDisplay);                   // Initialize the graphics LCD hardware and state
	clearGraphicsLcd(myDisplay);          		   // Clear the display buffer and the physical screen
	setGraphicsLcdTextPosition(myDisplay, 0, 0);  // Set the text cursor to the top-left corner
	putsGraphicsLcd(myDisplay, "Display Ready");  // Print "Display Ready" on the LCD
}

enum states
{
	DISPLAY,
	SETTINGS,
	SCREENSAVER
};

void consumerStateMachine()
{
	states state = DISPLAY;
	switch(state)
	{
		case DISPLAY: 
			displayTask();
			break;
		
		case SETTINGS:


	}
}

void displayTask()
{
    DisplayContext myDisplay;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * shmHandle = getShmHandle();

	//uint32_t i = 0;

    while (1)
	{

		sleep(1000);


		putsUart0("Temperature: ");
		putiUart0(shmHandle->temperature);
		putcUart0('\n');

		putsUart0("Turbidity: ");
		putiUart0(shmHandle->turbidity);
		putcUart0('\n');

		//drawGraphicsLcdRectangle(&myDisplay, 10, 10, 50, 20, INVERT); // Draw a rectangle on the display
//        setGraphic();
//		setGraphicsLcdTextPosition(&myDisplay, 20, 2);                 // Set text position
//		// putsGraphicsLcd(lcdHandler, "Task Running");                   // Print "Task Running" at the set position
//
//        lock(resource);
//        i++;
//        unlock(resource);
//        putsGraphicsLcd(lcdHandler, "NUM: ");
//        putiGraphicsLcd(lcdHandler, i);
//        sleep(1000);
//        clearGraphicsLcd(lcdHandler);
	}
}

void settingsTask()
{
    DisplayContext myDisplay;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * shmHandle = getShmHandle();

	//uint32_t i = 0;

    while (1)
	{

		sleep(1000);


		putsUart0("Temperature: ");
		putiUart0(shmHandle->temperature);
		putcUart0('\n');

		putsUart0("Turbidity: ");
		putiUart0(shmHandle->turbidity);
		putcUart0('\n');
	}
}

void screensaverTask()
{
    DisplayContext myDisplay;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * shmHandle = getShmHandle();

	//uint32_t i = 0;

    while (1)
	{

		sleep(1000);


		putsUart0("Temperature: ");
		putiUart0(shmHandle->temperature);
		putcUart0('\n');

		putsUart0("Turbidity: ");
		putiUart0(shmHandle->turbidity);
		putcUart0('\n');
	}
}
