/*
 * servicecalls.c
 *
 *  Created on: Sep 24, 2025
 *      Author: clara
 */

#include "servicecalls.h"

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

void * getShmHandle(void * ptr)
{
    __asm("SVCCALL24: SVC #24");
    return ptr;
}

void shmPerms()
{
    __asm("SVCCALL25: SVC #25");
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
// there's a crash here with pidof, assembly creates + 1 byte to the stack without decrement.
//_fn pidof(const char name[]){
//    __asm("SVCCALL11: SVC #11");
//    __asm("           MOV R0, R1"); // despite all my rage I'm still just a rat in a cage
//    __asm("           BX LR"); // Get cooking
//    return 0; // Who says I can't just return.... Twice... (I know I could have made an assembly file I'm lazy)
//}

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

