// RTOS Framework - Fall 2023
// D McComas

// Student Name:
// TO DO: Add your name(s) on this line.
//        Do not include your ID number(s) in the file.

// Please do not change any function name in this code or the thread priorities

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// 6 Pushbuttons and 5 LEDs, UART
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1
// Memory Protection Unit (MPU):
//   Region to control access to flash, peripherals, and bitbanded areas
//   4 or more regions to allow SRAM access (RW or none for task)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "dependencies/tm4c123gh6pm.h"
#include "dependencies/wait.h"

#include "kernel/mm.h"
#include "kernel/kernel.h"
#include "kernel/faults.h"

#include "drivers/clock.h"
#include "drivers/gpio.h"
#include "drivers/uart0.h"
#include "drivers/watchdog.h"

#include "processes/tasks.h"
#include "processes/shell.h"
#include "processes/turbidity.h"
#include "processes/display.h"
#include "processes/temperature.h"

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    bool ok;
    // Initialize hardware
    initHw();
    //initWatchdog();
    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);
    putsUart0("\033[2J"); // Clear screen after system start
    putsUart0("!!!! INIT !!!!\n");
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_PRIVDEFEN; // Plopping the background region init here for now till I find a better spot for it
    allowFlashAccess();
    allowPeripheralAccess();
    setupSramAccess();
    initRtos();
    putsUart0("Enabling MPU....\n");
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE;
    putsUart0("Done!\n\n");

    putsUart0("Initializing Semaphores and Mutexes....\n");
    // Initialize mutexes and semaphores
    initMutex(resource);
    initSemaphore(keyPressed, 1);
    initSemaphore(keyReleased, 0);
    initSemaphore(flashReq, 5);
    putsUart0("Done!\n\n");

    putsUart0("Loading system tasks....\n");
    // Add required idle process at lowest priority
    ok =  createThread(idle, "Idle", 7, 512);
    // Add other processes
//    ok &= createThread(lengthyFn, "LengthyFn", 6, 1024);
//    ok &= createThread(flash4Hz, "Flash4Hz", 4, 1024);
//    ok &= createThread(oneshot, "OneShot", 2, 1024);
//    ok &= createThread(readKeys, "ReadKeys", 6, 1024);
//    ok &= createThread(debounce, "Debounce", 6, 1024);
//    ok &= createThread(important, "Important", 0, 1024);
//    ok &= createThread(uncooperative, "Uncoop", 6, 1024);
//    ok &= createThread(errant, "Errant", 6, 1024);

    ok &= createThread(turbidityTask, "Turbidity", 0, 1024);
    ok &= createThread(displayTask, "Display", 6, 1024);
    ok &= createThread(tempTask, "Temperature", 6, 1024);

    ok &= createThread(shell, "Shell", 6, 8192);
    ok &= createThread(restartShell, "RestartShell", 6, 512);
    putsUart0("Done!\n\n");

    // TODO: Add code to implement a periodic timer and ISR

    putsUart0("Booting....\n\n");

    // Start up RTOS
    if (ok)
        startRtos(); // never returns
    else
        while(true);
}
