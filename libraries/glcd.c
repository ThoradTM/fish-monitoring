// Graphics LCD Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Keyboard Interface
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// ...existing code...
// ST7565R Graphics LCD Display Interface:
//   MOSI on PD3 (SSI1Tx)
//   SCLK on PD0 (SSI1Clk)
//   ~CS on PD1 (SSI1Fss)
//   A0 connected to PD2

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "clock.h"
#include "graphics_lcd.h"
#include "wait.h"
#include "tm4c123gh6pm.h"

// ...existing code...

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R3 | SYSCTL_RCGCGPIO_R4;
    _delay_cycles(3);

    // ...existing code...
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();

    // ...existing code...

    // Initialize graphics LCD
    initGraphicsLcd();

    // Draw X in left half of screen
    uint8_t i;
    for (i = 0; i < 64; i++)
        drawGraphicsLcdPixel(i, i, SET);
    for (i = 0; i < 64; i++)
        drawGraphicsLcdPixel(63-i, i, INVERT);

    // Draw text on screen
    setGraphicsLcdTextPosition(84, 5);
    putsGraphicsLcd("Text");

    // Draw flashing block around the text
    while(true)
    {
        drawGraphicsLcdRectangle(83, 39, 25, 9, INVERT);
        waitMicrosecond(500000);
    }
}
