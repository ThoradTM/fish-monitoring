// Tasks
// D McComas

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef DISPLAY_H_
#define DISPLAY_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

#include <stdint.h>

void initDisplay();

void consumerLoop();

uint8_t readPbs();


#endif
/*
 * display.h
 *
 *  Created on: Sep 19, 2025
 *      Author: clara
 */




