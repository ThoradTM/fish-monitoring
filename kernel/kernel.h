// Kernel functions
// D McComas

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef KERNEL_H_
#define KERNEL_H_

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// mutex
#define MAX_MUTEXES 1
#define MAX_MUTEX_QUEUE_SIZE 2
#define resource 0

// semaphore
#define MAX_SEMAPHORES 3
#define MAX_SEMAPHORE_QUEUE_SIZE 2
#define keyPressed 0
#define keyReleased 1
#define flashReq 2

// tasks
#define MAX_TASKS 12


#define MAX_ALLOCATIONS 3

typedef struct _taskctrl // Sorry I had to move this here, I figured that rewriting my malloc in the last week was a horrible idea
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    bool boosted;
    uint32_t ticks;                // ticks until sleep complete
    uint32_t pingPongA;
    uint32_t pingPongB;
    uint32_t timeDifferential;
    uint32_t stackBytes;
    uint8_t numOfAllocations;
    void * allocations[MAX_ALLOCATIONS];
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
}taskctrl;

extern uint8_t taskCurrent;
extern taskctrl tcb[MAX_TASKS];
extern bool RESET;

#define WORD_NUM 6

// Used for intertask communication
typedef struct _messenger{
    uint8_t i; // If needed this handles iteration for a system call.
    uint32_t words[WORD_NUM];
}messenger;

//-----------------------------------------------------------------------------
// Assembly Subroutines
//-----------------------------------------------------------------------------

#include "../assembly/asm_functions.h" // Refer to this file for assembly functions


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


// Kernel and Task Initialization
bool initMutex(uint8_t mutex);
bool initSemaphore(uint8_t semaphore, uint8_t count);
void initRtos(void);
void startRtos(void);
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);


// ISRs
void systickIsr(void);
void pendSvIsr(void);
void svCallIsr();


#endif
