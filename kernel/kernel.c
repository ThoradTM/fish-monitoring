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

/* GRADING NOTES:
 *
 * Pin assignments are set as the DEFAULT for the file I was given.
 *
 * My pushbutton code is NOT the same as was given, it was modified and I'm not undoing that because I don't have time.
 *
 * The TCB has been moved to a header file so mm.c can write allocated addresses to the TCB.
 *
 *
 *
 *
 *
 */


#include <stdint.h>
#include "../dependencies/tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "../drivers/uart0.h"
#include "../drivers/pain.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

uint8_t lockedBy = 0;
// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

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

// Standard System Calls

void yield(void)
{
    __asm("SVCCALL1: SVC #1");
}


void sleep(uint32_t tick)
{
    __asm("SVCCALL2: SVC #2");
}


void lock(int8_t mutex)
{
    __asm("SVCCALL3: SVC #3");
}


void unlock(int8_t mutex)
{
    __asm("SVCCALL4: SVC #4");
}


void wait(int8_t semaphore)
{
    __asm("SVCCALL5: SVC #5");
}


void post(int8_t semaphore)
{
    __asm("SVCCALL6: SVC #6");
}

void * mallocUnprivFromHeap(void * ptr){
    __asm("SVCCALL7: SVC #7");
    return ptr;
}

void stopThread(_fn fn)
{
    __asm("SVCCALL14: SVC #14");
}
void restartThread(_fn fn)
{
    __asm("SVCCALL17: SVC #17");
}
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm("SVCCALL19: SVC #19");
}





//
// Shell System Calls (or handlers for more advanced functionality)
//

// difference of time of tasks vs time of kernel for total time
// Read last complete set only (A or B) after the complete set is frozen


void sched(bool prio_on){
    __asm("SVCCALL9: SVC #9");
    if(prio_on){
        putsUart0("Scheduler Priority\n");

    }
    else{
        putsUart0("Scheduler Round Robin\n");
    }
}

void preempt(bool on){
    __asm("SVCCALL10: SVC #10");
    if(on){
        putsUart0("Preemption on\n");
    }
    else{
        putsUart0("Preemption off\n");
    }
}
// there's a crash here with pidof
_fn pidof(const char name[]){
    __asm("SVCCALL11: SVC #11");
    __asm("           MOV R0, R1"); // despite all my rage I'm still just a rat in a cage
    __asm("           BX LR");
    return 0; // Who says I can't just return.... Twice... (I know I could have made an assembly file I'm lazy)
}

void ipcs_sem(){
    __asm("SVCCALL12: SVC #12");
}

void reboot(){
    __asm("SVCCALL13: SVC #13");
}

void killnum(uint8_t index){
    __asm("SVCCALL21: SVC #21");
}

void ps(messenger * handler){
    __asm("SVCCALL15: SVC #15");
}

void ipcs_mut(messenger * handler){
    __asm("SVCCALL16: SVC #16");
}

void meminfo(messenger * handler){
    __asm("SVCCALL8: SVC #8");
}

void meminfoTask(messenger * handler){
    __asm("SVCCALL23: SVC #23");
}

void nameOf(uint32_t pid, char name[])
{
    __asm("SVCCALL18: SVC #18");
    putsUart0(name);
}

void saveShell(){
    __asm("SVCCALL20: SVC #20");
}

void pibool(bool on){ // priority inheritance (last thing I need to worry about)
    __asm("SVCCALL22: SVC #22");
    if(on){
        putsUart0("PI Enabled\n");
    }
    else{
        putsUart0("PI Disabled\n");
    }
};


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

    taskCurrent = rtosScheduler(); // Grab the next task

    tcb[taskCurrent].timeDifferential = timer;

    tcb[taskCurrent].sp = popContext(tcb[taskCurrent].sp, getMsp()); // Pop the old context and restore the updated sp
    __asm("LRRESTORE: MRS R0, PSP");
    __asm("           LDR LR, [R0, #4]!");

    // record systick timer start value

    applySramAccessMask(tcb[taskCurrent].srd); // check if this works
    setPsp(tcb[taskCurrent].sp); // Set the new PSP
}

bool svcUnlock = true; // Used to lock the first call to avoid an illegal call

void svCallIsr() //  fixed the other stuff, but this function is still kinda inefficient
{
    //uint8_t svcNumber = getSvc(); // From arm documentation
    uint8_t svcNumber = ((char *)getPsp()[6])[-2];
    uint8_t callArgs = *((uint8_t *)getPsp());
    uint32_t callArgs32 = *((uint32_t *)getPsp());
    uint8_t i;
    uint8_t j;
    uint32_t totalTime = 0;
    //uint32_t accumulator = TIMER5_TAV_R;
    //static uint8_t svcNumber = 0;
    switch(svcNumber){ // Trace back the stack and fetch the call number, then take a path based on it.
        case 0: // Start RTOS
            if(svcUnlock){
                tcb[taskCurrent].sp = popStart(tcb[taskCurrent].sp); // Pop the old context and restore the updated sp
                setPsp(tcb[taskCurrent].sp); // Set the new PSP
                applySramAccessMask(tcb[taskCurrent].srd);
                svcUnlock = false;
                NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE; // Moved this here to prevent a pendSv call before the OS is ready
            }
            break;
        case 1: // Yield
            pendSv();
            break;
        case 2: // Sleep
            tcb[taskCurrent].ticks = *(getPsp()); // Current R0 is the first function pass to sleep, SVCARGS is the PSP.
            tcb[taskCurrent].state = STATE_DELAYED; // Set the process state to delayed
            pendSv(); // Switch tasks
            break;
        case 3: // Lock
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
        case 4: //Unlock
            if(mutexes[callArgs].lock){ // PSP is pointing to R0 of last task which contains the passed mutex number
                mutexes[callArgs].lock = false;
                lockedBy = 0;
                mutexes[callArgs].lockedBy = 0;
                if(tcb[mutexes[callArgs].processQueue[0]].state == STATE_BLOCKED_MUTEX){
                    uint8_t taskNum = mutexes[callArgs].processQueue[0];
                    mutexes[callArgs].lock = true; // Give it the mutex
                    mutexes[callArgs].lockedBy = taskNum;
                    lockedBy = taskNum;
                    tcb[taskNum].state = STATE_READY;
                    uint8_t i;
                    for(i = MAX_MUTEX_QUEUE_SIZE; i > 0; i--){ // slide the queue down
                        mutexes[callArgs].processQueue[i-1] = mutexes[callArgs].processQueue[i];
                        mutexes[callArgs].processQueue[i] = 0;
                    }
                    mutexes[callArgs].queueSize--;
                    if(priorityInheritance && (tcb[taskCurrent].boosted == true)){
                        tcb[taskCurrent].priority = tcb[taskCurrent].currentPriority;
                        tcb[taskCurrent].boosted = false;
                    }
                }
            }
            break;
        case 5: // Wait
            if(semaphores[callArgs].count){
                semaphores[callArgs].count--;
            }
            else{
                semaphores[callArgs].processQueue[semaphores[callArgs].queueSize] = taskCurrent;
                tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
                semaphores[callArgs].queueSize++;
                pendSv();
            }
            break;
        case 6: // Post
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
            break;
        case 7:
            *((uint32_t *)getPsp()) = (uint32_t)mallocFromHeap(callArgs32);
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            tcb[taskCurrent].srd |= ~initSrdWindow(0); // Adding the new memory section, this function input doesn't matter
            tcb[taskCurrent].srd = ~tcb[taskCurrent].srd;
            applySramAccessMask(tcb[taskCurrent].srd);
            break;
        case 8: // meminfo
            ((messenger *)callArgs32)->words[0] = (uint32_t)systemmap.sectors; // ungodly creation
            ((messenger *)callArgs32)->words[1] = (uint32_t)(systemmap.sectors >> 32);
            ((messenger *)callArgs32)->words[2] = (uint32_t)systemmap.ownership[((messenger *)callArgs32)->i];
            break;
        case 9:
            priorityScheduler = *((uint32_t *)getPsp());
            break;
        case 10:
            preemption = *((uint32_t *)getPsp());
            break;
        case 11:
            pidOfPriv();
            break;
        case 12:
            ((messenger *)callArgs32)->words[0] = (uint32_t)semaphores[((messenger *)callArgs32)->i].count; // ungodly creation
            ((messenger *)callArgs32)->words[1] = (uint32_t)tcb[semaphores[((messenger *)callArgs32)->i].processQueue[0]].pid;
            ((messenger *)callArgs32)->words[2] = (uint32_t)tcb[semaphores[((messenger *)callArgs32)->i].processQueue[1]].pid;
            break;
        case 13:
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            break;
        case 14:
            stopThreadPriv((_fn)(*((uint32_t *)getPsp())));
            break;
        case 15: // Options: Copy a specific segment, or send it frame by frame.
            if((((messenger *)callArgs32)->i) == 0){
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
                ((messenger *)callArgs32)->words[3] = totalTime; // Total time to send to PS
            }

            ((messenger *)callArgs32)->words[0] = (uint32_t)tcb[((messenger *)callArgs32)->i].pid; // ungodly creation PID
            ((messenger *)callArgs32)->words[1] = tcb[((messenger *)callArgs32)->i].state;  // STATE
            ((messenger *)callArgs32)->words[2] = tcb[((messenger *)callArgs32)->i].priority; // PRIO

            if(!pingPong){ // Prevent this from running halfway through a flip
                ((messenger *)callArgs32)->words[4] = tcb[((messenger *)callArgs32)->i].pingPongA;
            }
            else{
                ((messenger *)callArgs32)->words[4] = tcb[((messenger *)callArgs32)->i].pingPongB;
            }
            break;
        case 16:
            ((messenger *)callArgs32)->words[0] = (uint32_t)mutexes[((messenger *)callArgs32)->i].lock; // ungodly creation
            ((messenger *)callArgs32)->words[1] = (uint32_t)tcb[mutexes[((messenger *)callArgs32)->i].lockedBy].pid;
            ((messenger *)callArgs32)->words[2] = (uint32_t)tcb[mutexes[((messenger *)callArgs32)->i].processQueue[0]].pid;
            ((messenger *)callArgs32)->words[3] = (uint32_t)tcb[mutexes[((messenger *)callArgs32)->i].processQueue[1]].pid;

            break;
        case 17:
            privRestartThread();
            break;
        case 18:
            nameOfPriv();
            break;
        case 19:
            setThreadPrioPriv((_fn)(*((uint32_t *)getPsp())), *((uint32_t *)getPsp() + 1));
            break;
        case 20:
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
        case 21:
            stopThreadIndexPriv(callArgs);
            break;
        case 22:
            priorityInheritance = *((uint32_t *)getPsp());
            break;
        case 23:
            ((messenger *)callArgs32)->words[0] = (uint32_t)tcb[((messenger *)callArgs32)->i].pid; // ungodly creation
            ((messenger *)callArgs32)->words[1] = (uint32_t)tcb[((messenger *)callArgs32)->i].allocations[0];
            ((messenger *)callArgs32)->words[2] = (uint32_t)tcb[((messenger *)callArgs32)->i].allocations[1];
            ((messenger *)callArgs32)->words[3] = (uint32_t)tcb[((messenger *)callArgs32)->i].allocations[2];
            ((messenger *)callArgs32)->words[4] = tcb[((messenger *)callArgs32)->i].stackBytes;
            break;
        default:
            break;
    }
// Check restart flash4hz check?
// Check mutex clear in stop task Good
// Change semaphore counts ????????????????????????????????????
// Weird pkill behaviour Above problem

//    if(pingPong){ // Good approximation of kernel time minus the very fast stuff
//        pingPongA += TIMER5_TAV_R - accumulator;
//    }
//    else{
//        pingPongB += TIMER5_TAV_R - accumulator;
//    }
}

