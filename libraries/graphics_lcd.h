// Graphics LCD Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// ST7565R Graphics LCD Display Interface:
//   MOSI on PD3 (SSI1Tx)
//   SCLK on PD0 (SSI1Clk)
//   ~CS on PD1 (SSI1Fss)
//   A0 connected to PD2

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef GRAPHICS_LCD_H_
#define GRAPHICS_LCD_H_

#include <stdint.h>

typedef struct _DisplayContext
{
    uint8_t buffer[1024];
    uint16_t txtIndex;
    // Add more fields as needed
} DisplayContext;

enum operation
{
    CLEAR,
    SET,
    INVERT
};

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void clearGraphicsLcd(DisplayContext * displayContext);
void initGraphicsLcd(DisplayContext * displayContext);
void drawGraphicsLcdPixel(DisplayContext * displayContext, uint8_t x, uint8_t y, enum operation op);
void drawGraphicsLcdRectangle(DisplayContext * displayContext, uint8_t xul, uint8_t yul, uint8_t dx, uint8_t dy, enum operation op);
void setGraphicsLcdTextPosition(DisplayContext * displayContext, uint8_t x, uint8_t page);
void putcGraphicsLcd(DisplayContext * displayContext, char c);
void putsGraphicsLcd(DisplayContext * displayContext, char str[]);
void putiGraphicsLcd(DisplayContext * displayContext, uint32_t x);

#endif

