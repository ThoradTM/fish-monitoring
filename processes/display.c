#include "display.h"               
#include "graphics_lcd.h"           


DisplayContext myDisplay;


void initDisplay()
{
	initGraphicsLcd(&myDisplay);                   // Initialize the graphics LCD hardware and state
	clearGraphicsLcd(&myDisplay);          		   // Clear the display buffer and the physical screen
	setGraphicsLcdTextPosition(&myDisplay, 0, 0);  // Set the text cursor to the top-left corner
	putsGraphicsLcd(&myDisplay, "Display Ready");  // Print "Display Ready" on the LCD
}


void displayTask()
{
	while (1) 
	{
		drawGraphicsLcdRectangle(&myDisplay, 10, 10, 50, 20, INVERT); // Draw a rectangle on the display
		setGraphicsLcdTextPosition(&myDisplay, 20, 2);                 // Set text position
		putsGraphicsLcd(&myDisplay, "Task Running");                   // Print "Task Running" at the set position
		
		for (volatile int i = 0; i < 1000000; i++); 				   // Add a delay to avoid rapid refresh 
	}
}
