/*
 * shell.c
 *
 *  Created on: Sep 1, 2024
 *      Author: Daniel
 */

#include "../libraries/commonui.h"
#include "../libraries/tm4c123gh6pm.h"

#include "../kernel/servicecalls.h"

#include "../drivers/uart0.h"
#include "../drivers/pain.h"
#include "../drivers/gpio.h"
#include "../kernel/shm.h"


char * procs[2] = {"hello", "world"};
uint8_t numprocs = 2;

void getStringUart(USER_DATA * data){
    int count = 0; // Make sure that count matches a valid index of the buffer
    char current_char = 0;
    while((current_char < 32) && (current_char != 13)){
        if(kbhitUart0()){
            current_char = getcUart0();
        }
        yield();
    }
    data->buffer[count] = current_char;
    count++;
    while(current_char != 13)
    {
        if(kbhitUart0()){// skip if uart0 rx fifo empty
            current_char = getcUart0();               // Fill char buffer with char from fifo w/ masked off flags
            if((current_char == 8) || (current_char == 127)){
                count--;
            }
            else{
                data->buffer[count] = current_char;
                count++;
                if(count == 80) break;
            }
        }
        yield();
    }
    data->buffer[count] = 0; //add null terminator to end of string
    return; // else return
}

uint8_t strlen(char * str){
    uint8_t i = 0;
    while(str[i] != '\0')
        i++;
    return i;
}


void shell(){
    USER_DATA data;
    messenger handler;
    putsUart0("\x1B[38;5;117m\nShell: \x1B[0m");
    shmPerms();
    shm * shmHandle = getShmHandle();
    shmHandle->presses = 0;
    shmHandle->presses2 = 0;
    shmHandle->feedingAmounts = 1;
    shmHandle->servoFlag = 0;
    while(true){
        data.fieldCount = 0;
        getStringUart(&data);
        parseFields(&data);
        if(isCommand(&data, "ps", 1)){ // test this later
            putsUart0("\x1B[;32mNUMBER\tNAME\t\tPID\tSTATE\tPRIORITY\tCPU TIME\n\x1B[0m");
            uint8_t i;
            uint32_t totalTime;
            for( i = 0; i < MAX_TASKS; i++ )
            {
                putiUart0(i);
                putcUart0('\t');
                handler.i = i;
                ps(&handler);
                char name[16];
                nameOf(handler.words[0], name);
                putcUart0('\t');
                if(strlen(name) < 8){
                    putcUart0('\t');
                }
                putiUart0(handler.words[0]); // PID
                putcUart0('\t');
                puthUart0(handler.words[1]); // STATE
                putcUart0('\t');
                puthUart0(handler.words[2]); // PRIORITY
                putcUart0('\t');
                putcUart0('\t');
                totalTime = handler.words[3];
                putcUart0('\t');
                if(totalTime){
                    putiUart0((handler.words[4]*10000)/totalTime / 100); // PING PONG
                    putcUart0('.');
                    putiUart0((handler.words[4]*10000)/totalTime % 100);
                    putcUart0('%');
                }
                putcUart0('\n');
            }
//            ps(&handler);
//            putsUart0("Kernel Time:");
//            putcUart0('\t');
//            totalTime = handler.words[3];
//            if(totalTime){
//                putiUart0(((40000000 - totalTime)/totalTime)/100); // PING PONG
//                putcUart0('.');
//                putiUart0(((40000000 - totalTime)/totalTime) % 100);
//                putcUart0('%');
//            }
//            putcUart0('\n');
        }
        else if(isCommand(&data, "reboot", 1)){
            reboot();
        }
        else if(isCommand(&data, "push", 1)){
            shmHandle->presses++;
        }
        else if(isCommand(&data, "pushmode", 1)){
            shmHandle->presses2++;
        }
        else if(isCommand(&data, "feedings", 2)){
            shmHandle->feedingAmounts = getFieldInteger(&data, 2);
        }
        else if(isCommand(&data, "ipcs", 1)){
            uint8_t i;
            putsUart0("-------- Mutex Arrays --------\n");
            for(i = 0; i < MAX_MUTEXES; i++){
                handler.i = i;
                ipcs_mut(&handler);
                putsUart0("Locked? ");
                putiUart0(handler.words[0]);
                putcUart0('\t');
                putsUart0("Locked by? ");
                putiUart0(handler.words[1]);
                if(handler.words[0]){
                    putcUart0('\n');
                    putsUart0("Queued:");
                    uint8_t j;
                    for(j = 2; j < (MAX_MUTEX_QUEUE_SIZE + 2); j++){
                        putcUart0('\t');
                        putiUart0(handler.words[j]);
                    }
                }
                putcUart0('\n');
            }
            putsUart0("-------- Semaphore Arrays --------\n");
            for(i = 0; i < MAX_SEMAPHORES; i++){
                handler.i = i;
                ipcs_sem(&handler);
                putsUart0("Available: ");
                putiUart0(handler.words[0]);
                putcUart0('\n');
                if(!handler.words[0]){
                    putsUart0("Queued:");
                    uint8_t j;
                    for(j = 1; j < MAX_SEMAPHORE_QUEUE_SIZE + 1; j++){
                        putcUart0('\t');
                        putiUart0(handler.words[j]);
                    }
                    putcUart0('\n');
                }
            }
        }
        else if(isCommand(&data, "meminfo", 1)){ // To do: Setup command to ALSO display addresses and which task owns what, additionally
            putsUart0("-------- Allocation Map --------\n");
            meminfo(&handler);
            uint8_t i;
            for(i = 0; i < 32; i++){
                if((handler.words[0] & 1) == 1){
                    putcUart0('1');
                }
                else{
                    putcUart0('0');
                }
                handler.words[0] >>= 1;
                putcUart0(' ');
            }
            for(i = 0; i < 32; i++){
                if(((handler.words[1]) & 1) == 1){
                    putcUart0('1');
                }
                else{
                    putcUart0('0');
                }
                handler.words[1] >>= 1;
                putcUart0(' ');
            }
            putcUart0('\n');
            putsUart0("-------- Region Size --------\n");
            for(i = 0; i < 40; i++){
                if(!(i % 5)){
                    putcUart0('\n');
                }
                else{
                    putcUart0('\t');
                }
                handler.i = i;
                meminfo(&handler);
                putiUart0(handler.words[2]);
            }
            putcUart0('\n');
        }
        else if(isCommand(&data, "meminfo", 2)){ // To do: Setup command to ALSO display addresses and which task owns what, additionally
            putsUart0("-------- Allocation Map --------\n");
            meminfo(&handler);
            uint8_t i;
            for(i = 0; i < 32; i++){
                if((handler.words[0] & 1) == 1){
                    putcUart0('1');
                }
                else{
                    putcUart0('0');
                }
                handler.words[0] >>= 1;
                putcUart0(' ');
            }
            for(i = 0; i < 32; i++){
                if(((handler.words[1]) & 1) == 1){
                    putcUart0('1');
                }
                else{
                    putcUart0('0');
                }
                handler.words[1] >>= 1;
                putcUart0(' ');
            }
            putcUart0('\n');
            putsUart0("-------- Region Size --------\n");
            for(i = 0; i < 40; i++){
                if(!(i % 5)){
                    putcUart0('\n');
                }
                else{
                    putcUart0('\t');
                }
                handler.i = i;
                meminfo(&handler);
                putiUart0(handler.words[2]);
            }
            putcUart0('\n');

            putsUart0("-------- Addresses and Ownership --------\n");
            for( i = 0; i < MAX_TASKS; i++ )
            {
                putiUart0(i);
                putcUart0('\t');
                handler.i = i;
                meminfoTask(&handler);
                char name[16];
                nameOf(handler.words[0], name);
                putcUart0('\t');
                if(strlen(name) < 8){
                    putcUart0('\t');
                }
                putcUart0('\n');
                putsUart0("Addresses Owned: ");
                puthUart0(handler.words[1]); // PID
                putcUart0(' ');
                puthUart0(handler.words[2]); // STATE
                putcUart0(' ');
                puthUart0(handler.words[3]); // PRIORITY
                putcUart0(' ');
                putcUart0('\n');
                putsUart0("Main Stack Size: ");
                puthUart0(handler.words[4]); // For a different usage arrangement
                putcUart0('\n');
                putcUart0('\n');
            }

        }
        else if(isCommand(&data, "setprio", 3)){
            setThreadPriority((_fn)getFieldInteger(&data, 2),getFieldInteger(&data, 3));
        }
        else if(isCommand(&data, "hex", 2)){
            puthUart0(getFieldInteger(&data, 2));
        }
        else if(isCommand(&data, "killnum", 2)){
            killnum((uint8_t)getFieldInteger(&data, 2));
        }
        else if(isCommand(&data, "kill", 2)){
            _fn pid = (_fn)getFieldInteger(&data, 2);
            stopThread(pid);
        }
        else if(isCommand(&data, "pi", 2)){
            char * command = getFieldString(&data, 2);
            if(!strcmp1(command,"ON")){
                pibool(1);
            }
            else{
                pibool(0);
            }
        }
        else if(isCommand(&data, "pkill", 2)){
            char * command = getFieldString(&data, 2);
            stopThread(pidof(command));
        }
        else if(isCommand(&data, "preempt", 2)){
            char * command = getFieldString(&data, 2);
            if(!strcmp1(command,"ON")){
                preempt(1);
            }
            else{
                preempt(0);
            }
        }
        else if(isCommand(&data, "sched", 2)){
            char * command = getFieldString(&data, 2);
            if(!strcmp1(command,"PRIO")){
                sched(1);
            }
            else{
                sched(0);
            }
        }
        else if(isCommand(&data, "pidof", 2)){
            char * command = getFieldString(&data, 2);
            putiUart0((uint32_t)pidof(command));
        }
        else if(isCommand(&data, "nameof", 2)){
            uint32_t command = getFieldInteger(&data, 2);
            char name[16];
            nameOf(command, name);
        }
        else{
            char * command = getFieldString(&data, 1);
            _fn pid = pidof(command); // Prevents some crashing
            if(pid){
                restartThread(pid);
            }
        }
        putsUart0("\x1B[38;5;117m\nShell: \x1B[0m");
    }
}


//putiUart0(((handler.words[4]*1000)/(totalTime*10 + (handler.words[5] >> 12)))/100); // PING PONG (Normalizing a Timer ISR measurement of kernel runtime)
//putcUart0('.');
//putiUart0(((handler.words[4]*1000)/(totalTime*10 + (handler.words[5] >> 12))) % 100);
//putcUart0('%');
//putcUart0('\n');
//}
//            handler.i = 200;
//            ps(&handler);
//            putsUart0("Kernel Time:");
//            putcUart0('\t');
//            putiUart0(((handler.words[5]*1000)/(totalTime*10 + (handler.words[5] >> 12)))/100); // KERNEL TIME
//            putiUart0(((handler.words[5]*1000)/(totalTime*10 + (handler.words[5] >> 12))) % 100); // KERNEL TIME
//            putcUart0('\n');
