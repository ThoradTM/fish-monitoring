// Kernel functions
// D McComas

// ALMOST ALL OF THE CODE IS ATLEAST TEMPLATED OR SOMEWHET SETUP, EEP TIME!


//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

/* Problematic comments:
 *
 * Pin assignments are set as the DEFAULT for the file I was given.
 *
 * My pushbutton code is NOT the same as was given, it was modified and I'm not undoing that because I don't have time.
 *
 * The TCB has been moved to a header file so mm.c can write allocated addresses to the TCB.
 *
 * Weird pkill behaviour Above problem
 *
 *
 */

#include <stdint.h>
#include "../libraries/tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "shm.h"
#include "../drivers/uart0.h"
#include "../drivers/pain.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// Mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

uint8_t lockedBy = 0;

// Semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// SHM
uint32_t * shm_struct_ptr;
uint64_t shm_srd;

enum
{
    STATE_INVALID,            // no task
    STATE_STOPPED,            // stopped, all memory freed
    STATE_READY,              // has run, can resume at any time
    STATE_DELAYED,            // has run, but now awaiting timer
    STATE_BLOCKED_MUTEX,      // has run, but now blocked by semaphore
    STATE_BLOCKED_SEMAPHORE   // has run, but now blocked by semaphore
};

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = true;          // preemption (true) or cooperative (false)
bool pingPong = false;

// tcb (redacted)
#define NUM_PRIORITIES   16

taskctrl tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void pendSv(){ // Placing this guy here for now
    NVIC_INT_CTRL_R |= (0x1 << 28);
}

uint8_t findTaskNum(void * taskToFind){
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){
        if(tcb[i].pid == taskToFind){
            return i;
        }
    }
    return 0;
}

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
        mutexes[mutex].queueSize = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
        semaphores[semaphore].queueSize = 0;
    }
    return ok;
}

void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }
}


uint8_t rtosScheduler(void)
{
    if(priorityScheduler){
        bool ok;
        bool lowest;
        static uint8_t task = 0xFF;
        //static uint8_t fifo[MAX_TASKS];
        uint8_t lowestPrio = 0;
        uint8_t iter = 0xFF;
        lowest = false;
        while(!lowest){
            iter++;
            lowest = (iter >= MAX_TASKS);
            if((tcb[iter].priority < tcb[lowestPrio].priority) && (tcb[iter].state == STATE_READY)){
                lowestPrio = iter;
            }
        }
        ok = false;
        while (!ok) // Quick and dirty for now
        {
            task++;
            if (task >= MAX_TASKS){
                task = 0;
            }
            ok = ((tcb[task].priority == tcb[lowestPrio].priority) && (tcb[task].state == STATE_READY));
        }
        return task;
    }
//        while (!ok) // Quick and dirty for now
//        {
//            task++;
//            if (task >= MAX_TASKS){
//                task = 0;
//                ok = true;
//            }
//            if((tcb[task].priority <= tcb[lowestPrio].priority) && (tcb[task].state == STATE_READY)){
//                lowestPrio = task;
//            }
//        }
//        return lowestPrio;
    else{
        bool ok;
        static uint8_t task = 0xFF;
        ok = false;
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
        return task;
    }
}


// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    // (ask leo)
    // Monumentally confused, should have gone into lab more recently.
    setPsp((void *)0x20008000); // Set the new PSP
    applySramAccessMask(0xFFFFFF00FFFFFFFF); // check if this works
    setAsp();
    setUnPriv();
    __asm("SVCCALL0: SVC #0");
}


// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
uint64_t initSrdWindow(uint32_t stackBytes){
    uint64_t sramAccess = createNoSramAccessMask();
    addSramAccessWindow(&sramAccess,0,0);
    return sramAccess;
}

// stack pointer is now size of stack - 32 bytes above base address
// written 8 things to memory

#define STRLEN 16

void dirtyStrCpy(const char str1[], char str2[]){
    uint8_t i;
    for(i = 0; i < STRLEN; i++){
        str2[i] = str1[i];
    }
}

bool restart = false;
bool mutex_ct_locked = false; // Adding this for a privileged application


bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{

    bool ok = false;
    uint8_t temp = taskCurrent; // UNBRIDLED RAGE AGAINST MISCOMMUNICATION
    uint8_t i = 0;
    bool found = false;
    if ((taskCount < MAX_TASKS) && !mutex_ct_locked)
    {
        mutex_ct_locked = true;
        // make sure fn not already in list (prevent reentrancy)
        while ((!found && (i < MAX_TASKS)) && !restart)
        {
            found = (tcb[i++].pid == fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;

            if(restart){
                while ((fn != tcb[i].pid) || (tcb[i].state != STATE_STOPPED)) {i++;} // ask leo about OR stop
            }
            else
                while (tcb[i].state != STATE_INVALID) {i++;} // ask leo about OR stop
            taskCurrent = i;
            tcb[i].sp = (uint8_t*)mallocFromHeap(stackBytes) + stackBytes;
            if(tcb[i].sp){ // Trying to prevent a system crash here when a task can't be created w/ memory
                tcb[i].state = STATE_READY;
                tcb[i].pid = fn;
                dirtyStrCpy(name,tcb[i].name);
                tcb[i].pingPongA = 0;
                tcb[i].pingPongB = 0;
                tcb[i].stackBytes = stackBytes;

                //tcb[i].sp = malloc_from_heap(stackBytes);
                tcb[i].spInit = tcb[i].sp; // Useful for debugging
                tcb[i].srd = initSrdWindow(stackBytes); // Doing this for now, need to figure out a better handler for SRD masks later.

                // Added shared memory space
                //tcb[i].srd |=


                // I'll enable mutexes when I get there

                // put a mutex on malloc till initSrdWindow is called?

                uint32_t * temp = tcb[i].sp;

                *(--temp) = (0x1 << 24); // xPSR, set thumb bit
                *(--temp) = (uint32_t)fn; // PC
                *(--temp) = 0;
                *(--temp) = 0; // R12
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;// If this is in a for loop,  my SP gets corrupted. Thanks C compiler.
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;
                *(--temp) = 0;

                tcb[i].sp = (void*)temp;

                //The task has run before (I promise)

                tcb[i].priority = priority;

                putsUart0((char *)name);
                putsUart0(" Loaded with PID ");
                putiUart0((uint32_t)fn);
                putsUart0(", Priority ");
                putiUart0((uint32_t)priority);
                putsUart0(", At SP ");
                puthUart0((uint32_t)tcb[i].sp);
                putcUart0('\n');

                tcb[i].timeDifferential = 0;

                tcb[i].boosted = false;
                tcb[i].pingPongA = 0;
                tcb[i].pingPongB = 0;

                // increment task count
                taskCount++;
                ok = true;
            }
        }
        mutex_ct_locked = false;
    }
    taskCurrent = temp;
    return ok;
}

// Privileged function helpers

void privRestartThread(){
    void * psp = getPsp();
    restart = true; // Set the restart flag to change create thread mode
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){ /// Find the task I want
        if((*((uint32_t *)psp) == (uint32_t)tcb[i].pid) && (tcb[i].state == STATE_STOPPED)){
            createThread((_fn)tcb[i].pid,tcb[i].name,tcb[i].priority,tcb[i].stackBytes); // Restart the thread
            i = MAX_TASKS;
        }
    }
    restart = false; // Reset the flag
}
// there's a crash here with pidof
void pidOfPriv(){
    uint32_t * psp = getPsp();
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){
        if(!strcmp1((char *)(*psp), tcb[i].name)){
            (*(psp + 1)) = (uint32_t)tcb[i].pid;
            i = MAX_TASKS;
        }
    }
}

void nameOfPriv(){
    uint32_t * psp = getPsp();
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){
        if((*psp) == (uint32_t)tcb[i].pid){
            dirtyStrCpy(tcb[i].name, (char *)(*(psp + 1)));
            i = MAX_TASKS;
        }
    }
}

uint8_t _indexOf(_fn pid){
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){
        if(pid == (_fn)tcb[i].pid){
            return i;
        }
    }
    return 0xFF;
}

void stopThreadPriv(_fn pid){
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){
        if(pid == (_fn)tcb[i].pid){
            //Perform KILL
            tcb[i].state = STATE_STOPPED;
            uint8_t j;
            for(j = 0; j < tcb[i].numOfAllocations; j++){
                freeToHeap(tcb[i].allocations[j]);
            }
            tcb[i].numOfAllocations = 0;
            tcb[i].pingPongA = 0;
            tcb[i].pingPongB = 0;
            uint8_t temp = taskCurrent;
            taskCurrent = i;
            if(i == lockedBy){ // WE LOVE WET CODING I LOVE COPY PASTING WHAT MAY OR MAY NOT WORK I HATE MAKING FUNCTIONS ADD MORE LINES
                if(mutexes[resource].lock){ // PSP is pointing to R0 of last task which contains the passed mutex number
                    mutexes[resource].lock = false;
                    lockedBy = 0;
                    mutexes[resource].lockedBy = 0;
                    if(tcb[mutexes[resource].processQueue[0]].state == STATE_BLOCKED_MUTEX){
                        uint8_t taskNum = mutexes[resource].processQueue[0];
                        mutexes[resource].lock = true; // Give it the mutex
                        mutexes[resource].lockedBy = taskNum;
                        lockedBy = taskNum;
                        tcb[taskNum].state = STATE_READY;
                        uint8_t i;
                        for(i = MAX_MUTEX_QUEUE_SIZE; i > 0; i--){ // slide the queue down
                            mutexes[resource].processQueue[i-1] = mutexes[resource].processQueue[i];
                            mutexes[resource].processQueue[i] = 0;
                        }
                        mutexes[resource].queueSize--;
                        if(priorityInheritance && (tcb[taskCurrent].boosted == true)){
                            tcb[taskCurrent].priority = tcb[taskCurrent].currentPriority;
                            tcb[taskCurrent].boosted = false;
                        }
                    }
                }
            }
            taskCurrent = temp;
            taskCount--;
            i = MAX_TASKS;
        }
    }
}

void stopThreadIndexPriv(uint8_t i){
    if(tcb[i].state != STATE_STOPPED){
        //Perform KILL
        tcb[i].state = STATE_STOPPED;
        uint8_t j;
        for(j = 0; j < tcb[i].numOfAllocations; j++){
            freeToHeap(tcb[i].allocations[j]);
        }
        tcb[i].numOfAllocations = 0;
        tcb[i].pingPongA = 0;
        tcb[i].pingPongB = 0;
        uint8_t temp = taskCurrent;
        taskCurrent = i;
        if(i == lockedBy){
            if(mutexes[resource].lock){ // PSP is pointing to R0 of last task which contains the passed mutex number
                mutexes[resource].lock = false;
                lockedBy = 0;
                mutexes[resource].lockedBy = 0;
                if(tcb[mutexes[resource].processQueue[0]].state == STATE_BLOCKED_MUTEX){
                    uint8_t taskNum = mutexes[resource].processQueue[0];
                    mutexes[resource].lock = true; // Give it the mutex
                    mutexes[resource].lockedBy = taskNum;
                    lockedBy = taskNum;
                    tcb[taskNum].state = STATE_READY;
                    uint8_t i;
                    for(i = MAX_MUTEX_QUEUE_SIZE; i > 0; i--){ // slide the queue down
                        mutexes[resource].processQueue[i-1] = mutexes[resource].processQueue[i];
                        mutexes[resource].processQueue[i] = 0;
                    }
                    mutexes[resource].queueSize--;
                    if(priorityInheritance && (tcb[taskCurrent].boosted == true)){
                        tcb[taskCurrent].priority = tcb[taskCurrent].currentPriority;
                        tcb[taskCurrent].boosted = false;
                    }
                }
            }
        }
        taskCurrent = temp;
        taskCount--;
    }
}

void setThreadPrioPriv(_fn fn, uint8_t priority){
    uint8_t i = 0xFF;
    while(fn != tcb[i].pid){
        i++;
        if (i >= MAX_TASKS)
            i = 0;
        if(tcb[i].pid == fn){
            tcb[i].priority = priority;
        }
    }
}

uint16_t timer = 0;
bool PREEMPT = true;

uint16_t idlecount = 0;

uint32_t pingPongA = 0;
uint32_t pingPongB = 0;

void systickIsr(void)
{
    uint8_t i;
    for(i = 0; i < MAX_TASKS; i++){ // Cycle through all tasks
        if(tcb[i].state == STATE_DELAYED){ // If it's delayed (sleep) decrement its tick count
            tcb[i].ticks--;
            if(tcb[i].ticks == 0){ // When its done sleeping ready the task
                tcb[i].state = STATE_READY;
            }
        }
    }
    if(preemption){
        pendSv();
    }

    timer++;
    if(timer >= 1000){ // All I have to do in Systick :)
        timer = 0;
        idlecount = 0;
        pingPong ^= true;

        if(pingPong){
            for(i = 0; i < MAX_TASKS; i++){ // Cycle through all tasks
                tcb[i].pingPongA = 0;
            }
        }
        else{
            for(i = 0; i < MAX_TASKS; i++){ // Cycle through all tasks
                tcb[i].pingPongB = 0;
            }
        }
    }
}

void pendSvIsr(void) // record
{
    // record systick timer value for end
    __asm("LRPUSH: MRS R0, PSP");
    __asm("        STR LR, [R0, #-4]!");
    tcb[taskCurrent].sp = pushContext(getPsp(), getMsp()); // Push the context. Store new program counter in old task with the PSP pointing to the top of the new stack


    if(NVIC_FAULT_STAT_R & 0x00000001){ // idle is getting corrupted during the thread re-starting for some reason
        stopThreadPriv((_fn)tcb[taskCurrent].pid);
        putsUart0("\x1B[;32mPendSv: Task ");
        putiUart0((uint32_t)tcb[taskCurrent].pid);
        putsUart0(" Killed\n\x1B[0m");
        NVIC_FAULT_STAT_R |= (0x00000001);
    }
    if(NVIC_FAULT_STAT_R & 0x00000002){ // Need to kill task that caused this here
        stopThreadPriv((_fn)tcb[taskCurrent].pid);
        putsUart0("\x1B[;32mPendSv: Task ");
        putiUart0((uint32_t)tcb[taskCurrent].pid);
        putsUart0(" Killed\n\x1B[0m");
        NVIC_FAULT_STAT_R |= (0x00000002);
    }



// No OS usage but super accurate system time
    if(tcb[taskCurrent].timeDifferential > timer){
        tcb[taskCurrent].timeDifferential = (1000 + (timer - tcb[taskCurrent].timeDifferential)); // Checking for roll over
    }
    else{
        tcb[taskCurrent].timeDifferential = timer - tcb[taskCurrent].timeDifferential;
    }

    if(pingPong){
        tcb[taskCurrent].pingPongA += tcb[taskCurrent].timeDifferential;
    }
    else{
        tcb[taskCurrent].pingPongB += tcb[taskCurrent].timeDifferential;
    }

    //    if(pingPong){ // Good approximation of kernel time minus the very fast stuff
    //        pingPongA += TIMER5_TAV_R - accumulator;
    //    }
    //    else{
    //        pingPongB += TIMER5_TAV_R - accumulator;
    //    }

    taskCurrent = rtosScheduler(); // Grab the next task

    tcb[taskCurrent].timeDifferential = timer;

    tcb[taskCurrent].sp = popContext(tcb[taskCurrent].sp, getMsp()); // Pop the old context and restore the updated sp
    __asm("LRRESTORE: MRS R0, PSP");
    __asm("           LDR LR, [R0, #4]!");

    // record systick timer start value

    applySramAccessMask(tcb[taskCurrent].srd); // check if this works
    setPsp(tcb[taskCurrent].sp); // Set the new PSP
}

enum
{
    START_RTOS,         // 0
    YIELD,              // 1
    SLEEP,              // 2
    LOCK_MUTEX,         // 3
    UNLOCK_MUTEX,       // 4
    WAIT,               // 5
    POST,               // 6
    MALLOC,             // 7
    MEMINFO,            // 8
    SET_SCHED,          // 9
    SET_PREEMPT,        // 10
    PIDOF,              // 11
    IPCS,               // 12
    REBOOT,             // 13
    STOP_THREAD,        // 14
    PS,                 // 15
    IPCS_MUT,           // 16
    RESTART_THREAD,     // 17
    NAMEOF,             // 18
    SET_PRIO,           // 19
    REBOOT_SHELL,       // 20
    KILL_NUM,           // 21
    SET_INHERIT_PRIO,   // 22
    MEMINFO_TASK,       // 23
    SHM_HANDLE,         // 24
    SHM_PERMS,          // 25
    SHM_DEL_PERMS       // 26
};

bool svcUnlock = true; // Used to lock the first call to avoid an illegal call

void svCallIsr() //  fixed the other stuff, but this function is still kinda inefficient
{
    // From arm documentation
    uint8_t svcNumber = ((char *)getPsp()[6])[-2];

    uint8_t callArgs = *((uint8_t *)getPsp());
    uint32_t callArgs32 = *((uint32_t *)getPsp());
    messenger * handler = ((messenger *)callArgs32);
    uint8_t i;
    uint8_t j;
    uint32_t totalTime = 0;

    // Trace back the stack and fetch the call number, then take a path based on it.

    switch(svcNumber)
    {
        case(START_RTOS): // Start RTOS
        {
            if(svcUnlock){
                tcb[taskCurrent].sp = popStart(tcb[taskCurrent].sp); // Pop the old context and restore the updated sp
                setPsp(tcb[taskCurrent].sp); // Set the new PSP
                applySramAccessMask(tcb[taskCurrent].srd);

                uint8_t i;
                for(i = 0; i < MAX_TASKS; i++){
                    if(!strcmp1("Shared Mem", tcb[i].name)){
                        break;
                    }
                }

                // Initial first pass at SHM. Will build a function to request access (SVC)
                // It will check a mutex lock before passing the shm
                shm_struct_ptr = (uint32_t *)((int)tcb[i].spInit - 256); // Grabs the middle of the range for SHM allocation
                shm_srd = tcb[i].srd;

                shm * shm_temp = (shm * )shm_struct_ptr;
                shm_temp->shared = 0;
                shm_temp->temperature = 0;
                shm_temp->turbidity = 0;


                svcUnlock = false;
                NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE; // Moved this here to prevent a pendSv call before the OS is ready
            }
            break;
        }

        case(YIELD): // Yield
        {
            pendSv();
            break;
        }

        case(SLEEP): // Sleep
        {
            tcb[taskCurrent].ticks = *(getPsp()); // Current R0 is the first function pass to sleep, SVCARGS is the PSP.
            tcb[taskCurrent].state = STATE_DELAYED; // Set the process state to delayed
            pendSv(); // Switch tasks
            break;
        }

        case(LOCK_MUTEX): // Lock
        {
            if(!mutexes[callArgs].lock){ // PSP is pointing to R0 of last task which contains the passed mutex number
                mutexes[callArgs].lock = true;
                mutexes[callArgs].lockedBy = taskCurrent;
                lockedBy = taskCurrent;
            }
            else{
                tcb[taskCurrent].mutex = callArgs; //Which mutex is being locked
                tcb[taskCurrent].state = STATE_BLOCKED_MUTEX; // Set the task as blocked
                if(priorityInheritance && (tcb[taskCurrent].priority < tcb[lockedBy].priority)){ // If PI is enabled and applies to the case
                    if(mutexes[callArgs].queueSize > 0){
                        for(i = 0; i < mutexes[callArgs].queueSize; i++){ // slide the queue down and have queue items inherit the priority
                            tcb[mutexes[callArgs].processQueue[i]].currentPriority = tcb[mutexes[callArgs].processQueue[i]].priority;
                            tcb[mutexes[callArgs].processQueue[i]].priority = tcb[taskCurrent].priority;
                            tcb[mutexes[callArgs].processQueue[i]].boosted = true;
                        }
                    }
                    tcb[lockedBy].currentPriority = tcb[lockedBy].priority; // set the priority of the holder to the task that needs it
                    tcb[lockedBy].priority = tcb[taskCurrent].priority;
                    tcb[lockedBy].boosted = true; // Notify that the current holder is boosted
                }
                mutexes[callArgs].processQueue[mutexes[callArgs].queueSize] = taskCurrent; // Add the task to the queue
                mutexes[callArgs].queueSize++;
                pendSv(); // Switch tasks so we don't return and illegally access the item
            }
            break;
        }

        case(UNLOCK_MUTEX): // Unlock
        {
            if(mutexes[callArgs].lock)
            { // PSP is pointing to R0 of last task which contains the passed mutex number
                mutexes[callArgs].lock = false;
                lockedBy = 0;
                mutexes[callArgs].lockedBy = 0;
                if(tcb[mutexes[callArgs].processQueue[0]].state == STATE_BLOCKED_MUTEX)
                {
                    uint8_t taskNum = mutexes[callArgs].processQueue[0];
                    mutexes[callArgs].lock = true; // Give it the mutex
                    mutexes[callArgs].lockedBy = taskNum;
                    lockedBy = taskNum;
                    tcb[taskNum].state = STATE_READY;
                    uint8_t i;
                    for(i = MAX_MUTEX_QUEUE_SIZE; i > 0; i--)
                    { // slide the queue down
                        mutexes[callArgs].processQueue[i-1] = mutexes[callArgs].processQueue[i];
                        mutexes[callArgs].processQueue[i] = 0;
                    }
                    mutexes[callArgs].queueSize--;
                    if(priorityInheritance && (tcb[taskCurrent].boosted == true))
                    {
                        tcb[taskCurrent].priority = tcb[taskCurrent].currentPriority;
                        tcb[taskCurrent].boosted = false;
                    }
                }
            }
            break;
        }

        case(WAIT): // Wait
        {
            if(semaphores[callArgs].count) semaphores[callArgs].count--;
            else
            {
                semaphores[callArgs].processQueue[semaphores[callArgs].queueSize] = taskCurrent;
                tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
                semaphores[callArgs].queueSize++;
                pendSv();
            }
            break;
        }

        case(POST): // Post
        {
                semaphores[callArgs].count++;
//                if(priorityInheritance && tcb[currentTask].boosted){
//                    tcb[currentTask].priority = tcb[currentTask].currentPriority;
//                    tcb[currentTask].boosted = false;
//                }
                if(semaphores[callArgs].queueSize){
                    tcb[semaphores[callArgs].processQueue[0]].state = STATE_READY;
                    uint8_t i;
                    for(i = MAX_SEMAPHORE_QUEUE_SIZE; i > 0; i--){ // slide the queue down
                        semaphores[callArgs].processQueue[i-1] = semaphores[callArgs].processQueue[i];
                        semaphores[callArgs].processQueue[i] = 0;
                    }
                    semaphores[callArgs].queueSize--;
                    semaphores[callArgs].count--;
                    //pendSv();
                }
//        case 6:
//                if(semaphores[callArgs].queueSize){
//                    tcb[semaphores[callArgs].processQueue[0]].state = STATE_READY;
//                    uint8_t i;
//                    for(i = MAX_MUTEXES; i > 0; i--){ // slide the queue down
//                        semaphores[callArgs].processQueue[i-1] = semaphores[callArgs].processQueue[i];
//                        semaphores[callArgs].processQueue[i] = 0;
//                    }
//                    semaphores[callArgs].queueSize--;
//                    pendSv();
//                }
//                semaphores[callArgs].count++;
//            break;
        }break;

        case(MALLOC):
        {
            *((uint32_t *)getPsp()) = (uint32_t)mallocFromHeap(callArgs32);
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            tcb[taskCurrent].srd |= ~initSrdWindow(0); // Adding the new memory section, this function input doesn't matter
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            applySramAccessMask(tcb[taskCurrent].srd);
            break;
        }

        case(MEMINFO): // Meminfo
        {
            handler->words[0] = (uint32_t)systemmap.sectors;
            handler->words[1] = (uint32_t)(systemmap.sectors >> 32);
            handler->words[2] = (uint32_t)systemmap.ownership[handler->i];
            break;
        }

        case(SET_SCHED):
        {
            priorityScheduler = *((uint32_t *)getPsp());
            break;
        }

        case(SET_PREEMPT):
        {
            preemption = *((uint32_t *)getPsp());
            break;
        }

        case(PIDOF):
        {
            pidOfPriv();
            break;
        }

        case(IPCS):
        {
            handler->words[0] = (uint32_t)semaphores[handler->i].count;
            handler->words[1] = (uint32_t)tcb[semaphores[handler->i].processQueue[0]].pid;
            handler->words[2] = (uint32_t)tcb[semaphores[handler->i].processQueue[1]].pid;
            break;
        }

        case(REBOOT):
        {
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            break;
        }

        case(STOP_THREAD):
        {
            stopThreadPriv((_fn)(*((uint32_t *)getPsp())));
            break;
        }

        case(PS): // Options: Copy a specific segment, or send it frame by frame.
        {
            if((handler->i) == 0){
                if(!pingPong){
                    for(i = 0; i < taskCount; i++){
                        totalTime += tcb[i].pingPongA;
                    }
                }
                else{
                    for(i = 0; i < taskCount; i++){
                        totalTime += tcb[i].pingPongB;
                    }
                }
                handler->words[3] = totalTime; // Total time to send to PS
            }

            handler->words[0] = (uint32_t)tcb[handler->i].pid;
            handler->words[1] = tcb[handler->i].state;  // STATE
            handler->words[2] = tcb[handler->i].priority; // PRIO

            if(!pingPong){ // Prevent this from running halfway through a flip
                handler->words[4] = tcb[handler->i].pingPongA;
            }
            else{
                handler->words[4] = tcb[handler->i].pingPongB;
            }
            break;
        }

        case(IPCS_MUT):
        {
            handler->words[0] = (uint32_t)mutexes[handler->i].lock;
            handler->words[1] = (uint32_t)tcb[mutexes[handler->i].lockedBy].pid;
            handler->words[2] = (uint32_t)tcb[mutexes[handler->i].processQueue[0]].pid;
            handler->words[3] = (uint32_t)tcb[mutexes[handler->i].processQueue[1]].pid;
            break;
        }

        case(RESTART_THREAD):
        {
            privRestartThread();
            break;
        }

        case(NAMEOF):
        {
            nameOfPriv();
            break;
        }

        case(SET_PRIO):
        {
            setThreadPrioPriv((_fn)(*((uint32_t *)getPsp())), *((uint32_t *)getPsp() + 1));
            break;
        }

        case(REBOOT_SHELL):
        {
            i = 0;
            for(j = 0; j < MAX_TASKS; j++){
                if(!strcmp1("Shell", tcb[i].name) && tcb[i].state == STATE_READY){
                    i++;
                }
            }
            if(i == 0){
                for(i = 0; i < MAX_TASKS; i++){
                    if(!strcmp1("Shell", tcb[i].name) && tcb[i].state == STATE_STOPPED){
                        putsUart0("Shell Crashed!\nRecovering.........");
                        restart = true;
                        createThread((_fn)tcb[i].pid,tcb[i].name,tcb[i].priority,tcb[i].stackBytes);
                        restart = false;
                    }
                }
            }
            break;
        }

        case(KILL_NUM):
        {
            stopThreadIndexPriv(callArgs);
            break;
        }

        case SET_INHERIT_PRIO:
        {
            priorityInheritance = *((uint32_t *)getPsp());
            break;
        }

        case(MEMINFO_TASK):
        {
            handler->words[0] = (uint32_t)tcb[handler->i].pid;
            handler->words[1] = (uint32_t)tcb[handler->i].allocations[0];
            handler->words[2] = (uint32_t)tcb[handler->i].allocations[1];
            handler->words[3] = (uint32_t)tcb[handler->i].allocations[2];
            handler->words[4] = tcb[handler->i].stackBytes;
            break;
        }

        case(SHM_HANDLE):
        {
            // I can copy the mutex code into here and have it perform all of that work on the back end.
            // No reason to re-write code.
            // This will require the handle to be closed out manually via a mutex post call.
            // Another way to do this is more messenger structs which is an option. That might be more complicated.
            // Even easier, make it an un-exposed service call and build a library that keeps track of a single mutex
            // As well as the masks, so a user can post (write to field x with x), it will wait on the sem in the lib,
            // then the write/read will pass.

            // Call order: Lock SHM mutex, grab pointer, apply access mask, perform operation, remove access mask,
            // unlock mutex.

            *((uint32_t *)getPsp()) = (uint32_t)shm_struct_ptr;
            break;
        }

        // Lock SHM
        case(SHM_PERMS):
        {
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            tcb[taskCurrent].srd |= ~shm_srd; // From malloc above
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            applySramAccessMask(tcb[taskCurrent].srd);
            break;
        }

        // Unlock SHM
        case(SHM_DEL_PERMS):
        {
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            tcb[taskCurrent].srd &= shm_srd; // Inversion of the above to remove access
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            applySramAccessMask(tcb[taskCurrent].srd);
            break;
        }

        default:
            break;
    }
}

