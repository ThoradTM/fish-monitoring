// Shell functions
// Daniel McComas

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "../libraries/tm4c123gh6pm.h"
#include "faults.h"
#include "kernel.h"
#include "../drivers/uart0.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file


static void
dump()
{ // MMADDR
    uint32_t * PSP = (uint32_t *)getPsp();
    puthUart0(*(PSP+6));
    putcUart0('\n');
    putsUart0("MSP: ");
    puthUart0((uint32_t)getMsp());
    putcUart0('\n');
    putsUart0("NvicFaultStat: ");
    puthUart0(NVIC_FAULT_STAT_R);
    putcUart0('\n');
    putsUart0("NvicHFaultStat: ");
    puthUart0(NVIC_HFAULT_STAT_R);
    putcUart0('\n');

    uint8_t i = 0;

    for(i = 0; i < 4; i++){
        putcUart0('R');
        putiUart0(i);
        putsUart0(": ");
        puthUart0(*(PSP+i));
        putcUart0('\n');
    }

    putsUart0("PSP: ");
    puthUart0((uint32_t)getPsp());
    putcUart0('\n');

    putcUart0('R');
    putiUart0(12);
    putsUart0(": ");
    puthUart0(*(PSP+4));
    putcUart0('\n');

    putsUart0("LR: ");
    puthUart0(*(PSP+5));
    putcUart0('\n');
    putsUart0("PC: ");
    puthUart0(*(PSP+6));
    putcUart0('\n');
    putsUart0("xPSR: ");
    puthUart0(*(PSP+7));
    putcUart0('\n');
    putsUart0("PID: ");
    putiUart0((uint32_t)tcb[taskCurrent].pid);
    putcUart0('\n');
}



// REQUIRED: code this function
void mpuFaultIsr(void)
{
    putcUart0('\n');
    putsUart0("\x1B[;31m!!!!MPU Fault!!!!\x1B[0m\n");
    putsUart0("\x1B[;32mOffender: \x1B[0m");
    uint32_t * PSP = (uint32_t *)getPsp();
    puthUart0(*(PSP+6));
    putcUart0('\n');
    putsUart0("\x1B[;32mOffending Address: \x1B[0m");
    puthUart0(NVIC_MM_ADDR_R);
    putcUart0('\n');
    dump();
    NVIC_INT_CTRL_R |= (0x1 << 28);
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    putcUart0('\n');
    putsUart0("\x1B[;31m!!!!Hard Fault!!!!\x1B[0m\n");
    putsUart0("\x1B[;32mOffender: \x1B[0m");

    dump();
    putcUart0('\n');
    putsUart0("\x1B[;32mOffending Address: \x1B[0m");
    puthUart0(NVIC_MM_ADDR_R);
    putcUart0('\n');


    while(1)
    {
    }
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    putcUart0('\n');
    putsUart0("\x1B[;31m!!!!Bus Fault!!!!\x1B[0m\n");
    putsUart0("\x1B[;32mOffender: \x1B[0m");
    dump();
    putcUart0('\n');
    putsUart0("\x1B[;32mOffending Address: \x1B[0m");
    puthUart0(NVIC_MM_ADDR_R);
    putcUart0('\n');
    while(1)
    {
    }
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    putcUart0('\n');
    putsUart0("\x1B[;31m!!!!Usage Fault!!!!\x1B[0m\n");
    putsUart0("\x1B[;32mOffender: \x1B[0m");
    dump();
    while(1)
    {
    }
}

