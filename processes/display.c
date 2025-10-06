#include "display.h"               
#include "../libraries/graphics_lcd.h"
#include "../kernel/shm.h"
#include "../kernel/kernel.h"



void initDisplay(DisplayContext * myDisplay)
{
	initGraphicsLcd(myDisplay);                   // Initialize the graphics LCD hardware and state
	clearGraphicsLcd(myDisplay);          		   // Clear the display buffer and the physical screen
	setGraphicsLcdTextPosition(myDisplay, 0, 0);  // Set the text cursor to the top-left corner
	putsGraphicsLcd(myDisplay, "Display Ready");  // Print "Display Ready" on the LCD
}


void displayTask()
{
    DisplayContext myDisplay;
    DisplayContext * lcdHandler = &myDisplay;
    initDisplay(lcdHandler);

	shmPerms();
    shm * test = getShmHandle();

	uint32_t i = 0;

    while (1)
	{
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
