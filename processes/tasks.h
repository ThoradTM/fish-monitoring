// Tasks
// D McComas

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef TASKS_H_
#define TASKS_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shm_task(); // Dummy task that holds on to a memory pointer, its only use is to create an SHM region.
// Bad practice but I need this to work.

void idle(void);

void shmTestWriter();
void shmTestReader();

void flash4Hz(void);
void oneshot(void);
void partOfLengthyFn(void);
void lengthyFn(void);
void readKeys(void);
void debounce(void);
void uncooperative(void);
void errant(void);
void important(void);
void idle2(void);
void timer1Isr();
void restartShell(void);
#endif
