// Tasks
// Dylan Nguyen

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef TURBIDITY_H_
#define TURBIDITY_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initTurbidity();
uint16_t readTurbidityRaw();
void turbidityTask();


#endif
