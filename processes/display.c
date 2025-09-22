#include "display.h"
#include "graphics_lcd.h"

void initDisplay()
{
	// Initialize the graphics LCD
	initGraphicsLcd();
	clearGraphicsLcd();
	setGraphicsLcdTextPosition(0, 0);
	putsGraphicsLcd("Display Ready");
}

DisplayContext myDisplay;

void displayTask()
{
	// Use DisplayContext struct for display state
	while (1)
	{
		drawGraphicsLcdRectangle(&myDisplay, 10, 10, 50, 20, INVERT);
		setGraphicsLcdTextPosition(&myDisplay, 20, 2);
		putsGraphicsLcd(&myDisplay, "Task Running");

		// Add a delay to avoid rapid refresh (adjust as needed)
		for (volatile int i = 0; i < 1000000; i++);
	}
}
