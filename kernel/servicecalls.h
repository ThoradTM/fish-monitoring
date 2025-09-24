#ifndef SERVICE_CALLS_H
#define SERVICE_CALLS_H

#include <stdint.h>
#include "kernel.h"

// System Calls
void yield(void);
void sleep(uint32_t tick);
void lock(int8_t mutex);
void unlock(int8_t mutex);
void wait(int8_t semaphore);
void post(int8_t semaphore);
void setThreadPriority(_fn fn, uint8_t priority);
void restartThread(_fn fn);
void stopThread(_fn fn);
void * mallocUnprivFromHeap(void * ptr);
void * getShmHandle(void * ptr);
void  shmPerms();

// Unprivileged shell commands
void sched(bool prio_on);
void preempt(bool on);
extern _fn pidof(const char name[]); // Assembly function placed here to avoid crashes from inline assembly.
void reboot();
void stopThread(_fn fn);
void ps(messenger * handler);
void ipcs_mut();
void meminfo(messenger * handler);
void ipcs_sem();
void saveShell();
void restartThread(_fn fn);
void nameOf(uint32_t pid, char name[]);
void kill(_fn fn);
void pibool(bool on);
void setThreadPriority(_fn fn, uint8_t priority);
void killnum(uint8_t index);
void meminfoTask(messenger * handler);

#endif
