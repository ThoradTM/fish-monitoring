// RTOS Framework - Fall 2023
// D McComas

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Memory Protection Unit (MPU):
//   Region to control access to flash, peripherals, and bitbanded areas
//   4 or more regions to allow SRAM access (RW or none for task)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "libraries/tm4c123gh6pm.h"
#include "libraries/wait.h"

#include "kernel/mm.h"
#include "kernel/kernel.h"
#include "kernel/faults.h"

#include "drivers/clock.h"
#include "drivers/gpio.h"
#include "drivers/uart0.h"
#include "drivers/watchdog.h"
#include "drivers/inithw.h"


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

//-----------------------------------------------------------------------------
// Init Hardware
//-----------------------------------------------------------------------------

    bool ok;
//    Initialize hardware
    initHw();
    tempInit();

//    initWatchdog();
//    Setup UART0 baud rate
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

//-----------------------------------------------------------------------------
// Semaphores & Mutexes
//-----------------------------------------------------------------------------

    putsUart0("Initializing Semaphores and Mutexes....\n");

//    initMutex(resource);
//    initSemaphore(keyPressed, 1);
//    initSemaphore(keyReleased, 0);
//    initSemaphore(flashReq, 5);

    putsUart0("Done!\n\n");

//-----------------------------------------------------------------------------
// Processes
//-----------------------------------------------------------------------------

    putsUart0("Loading system tasks....\n");

    ok =  createThread(shm_task, "Shared Mem", 15, 512);

    ok =  createThread(idle, "Idle", 6, 512);


    ok &= createThread(shmTestWriter, "shmTestWriter", 5, 512);
    ok &= createThread(shmTestReader, "shmTestReader", 5, 512);
//    ok &= createThread(lengthyFn, "LengthyFn", 6, 1024);
//    ok &= createThread(flash4Hz, "Flash4Hz", 4, 1024);
//    ok &= createThread(oneshot, "OneShot", 2, 1024);
//    ok &= createThread(readKeys, "ReadKeys", 6, 1024);
//    ok &= createThread(debounce, "Debounce", 6, 1024);
//    ok &= createThread(important, "Important", 0, 1024);
//    ok &= createThread(uncooperative, "Uncoop", 6, 1024);
//    ok &= createThread(errant, "Errant", 6, 1024);

    ok &= createThread(turbidityTask, "Turbidity", 6, 512);
    ok &= createThread(displayTask, "Display", 6, 4096);
    ok &= createThread(tempTask, "Temperature", 0, 512);
    ok &= createThread(shell, "Shell", 6, 1024);
    ok &= createThread(restartShell, "RestartShell", 6, 512);

    putsUart0("Done!\n\n");

//-----------------------------------------------------------------------------
// Kernel Init
//-----------------------------------------------------------------------------

    putsUart0("Booting....\n\n");

    if (ok)
        startRtos();
    else
        putsUart0("Kernel Init failed! Hanging.\n");
        while(true);
}
