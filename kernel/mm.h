// Memory manager functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef MM_H_
#define MM_H_

#include <stdint.h>

#define NUM_SRAM_REGIONS 4

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


typedef struct _map
{
    uint64_t sectors; // Showing that MPU regions 6 and 7 are reserved
    uint64_t mask; // Showing that MPU regions 6 and 7 are reserved
    uint8_t ownership[40];
}map;

extern map systemmap;


void * mallocFromHeap(uint32_t size_in_bytes);
void freeToHeap(void *pMemory);

void allowFlashAccess(void);
void allowPeripheralAccess(void);
void setupSramAccess(void);
uint64_t createNoSramAccessMask(void);
void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes);
void applySramAccessMask(uint64_t srdBitMask);

#endif
