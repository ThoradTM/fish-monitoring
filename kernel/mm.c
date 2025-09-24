// Memory manager functions
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
#include "mm.h"
#include "kernel.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


/*
 * malloc.c
 *
 *  Created on: Sep 12, 2024
 *      Author: clara
 */

/*
 * Memory Block:
 *
 * 28k
 * v
 * 4k
 * 4k
 * 4k
 * 4k
 * 4k
 * 4k
 * 4k
 * heap^
 * MSR
 * v
 * 4k
 *
 *
 *
 *
 */

/*
 * Memory layout:
 * One sector 4k with 8 sections of 512 for programs that use less than 512
 * 3 sectors of 8k with 8 sectors of 1024
 */

/*
 * Memory layout:
 * One sector 4k with 8 sections of 512 for programs that use less than 512
 * 3 sectors of 8k with 8 sectors of 1024
 *
 * Or any combination of 8 and 4k
 *
 * 4k can start anywhere
 * 8k has to start elsewhere because the OS takes 4k
 *
 */

/*
 * < 512 A
 * --------
 * > 512 B
 * <1024
 * --------
 * >1024 A-B
 * <1536
 * --------
 * >1536 (Multiple B blocks)
 */

/*
 * 4096 - 8 512k
 * 4096 - 8 512k
 * 8k - 81k
 * 4*1024 -> 8 128k
 * 4096 - 8 512k
 * 4096 - 8 512k
 */
// 2 8k - 16k
// 4 4k -,
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include "../drivers/clock.h"
#include "../drivers/pain.h"
#include "../processes/shell.h"
#include "../drivers/uart0.h"
#include "../libraries/tm4c123gh6pm.h"

uint8_t littleallocated = 0;
uint8_t bigallocated = 0;
//

extern uint32_t heap[7168];

map systemmap;

void initSystemMap(){
    systemmap.sectors = 0;
    systemmap.mask = 0;
    uint8_t i = 0;
    for(i = 0; i < 40; i++){
        systemmap.ownership[i] = 0;
    }
}


void * bigalloc(uint32_t size){
    uint16_t blocknum = size / 1024;
    uint8_t i;
    systemmap.mask = 0;
    uint64_t frame = 1;
    if((size % 1024) != 0)
        blocknum++;
    uint16_t ospacec = blocknum;
    systemmap.mask += 1;
    if(blocknum > 1){
        for(i = 0; i < (blocknum-1); i++){
            systemmap.mask <<= 1;
            systemmap.mask += 1;
        }
    }
    frame <<= 24;
    if(blocknum <= (16 - bigallocated)){
        for(i = 23; i < 40; i++){
            if(!ospacec){ // ISSUE HERE
                uint8_t offset = ((i - blocknum) + 1);
                uint8_t memoffset = (((i - 23) - blocknum));
                systemmap.mask <<= offset;
                uint8_t j;
                for(j = offset; j < (i+1); j++){
                    systemmap.ownership[offset]++;
                }
                bigallocated += blocknum;
                systemmap.sectors |= systemmap.mask;
                return (void *)(0x20004000 + memoffset*0x400);
            }
            uint64_t check = (frame & systemmap.sectors);
            if(!(check)){
                ospacec--;
            }
            else if(ospacec < blocknum){
                ospacec = blocknum;
            }
            frame <<= 1;
        }
        if(blocknum != 0){
            return 0;
        }
    }
    return 0;
}

void * littlealloc(uint32_t size){
    uint16_t blocknum = size / 512;
    uint8_t i;
    systemmap.mask = 0;
    uint64_t frame = 1;
    if((size % 512) != 0)
        blocknum++;
    uint16_t ospacec = blocknum;
    systemmap.mask += 1;
    if(blocknum > 1){
        for(i = 0; i < (blocknum-1); i++){
            systemmap.mask <<= 1;
            systemmap.mask += 1;
        }
    }
    if(blocknum <= (24 - littleallocated)){
        for(i = 0; i < 24; i++){
            if(!ospacec){
                uint8_t offset = (i - blocknum);
                systemmap.mask <<= (offset);
                uint8_t j;
                for(j = offset; j < i; j++){
                    systemmap.ownership[offset]++;
                }
                littleallocated += blocknum;
                systemmap.sectors |= systemmap.mask;
                return (void *)(0x20001000 + offset*0x200);
            }
            uint64_t check = (frame & systemmap.sectors);
            if(!(check)){
                ospacec--;
            }
            else if(ospacec < blocknum){
                ospacec = blocknum;
            }
            frame <<= 1;
        }
        if(blocknum != 0){
            return 0;
        }
    }
    return 0;
}






// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void * mallocFromHeap(uint32_t size_in_bytes)
{
    if(size_in_bytes > 512)
    {
        void * allocated_slice = bigalloc(size_in_bytes);
        if(allocated_slice){
            tcb[taskCurrent].allocations[tcb[taskCurrent].numOfAllocations] = allocated_slice;
            tcb[taskCurrent].numOfAllocations++;
            return allocated_slice;
        }
    }
    void * retval = littlealloc(size_in_bytes); // Littlealloc called once
    if(retval){
        tcb[taskCurrent].allocations[tcb[taskCurrent].numOfAllocations] = retval;
        tcb[taskCurrent].numOfAllocations++;
    }
    return retval; // A second time for some reason
}

// REQUIRED: add your free code here and update the SRD bits for the current thread
void freeToHeap(void *pMemory)
{ // bug: task current is not the correct boi for this
    uint8_t i;
    uint64_t mask = 1;
    for(i = 0; i < 40; i++){
        if(i > 23){
            if(((void *)(0x20004000 + (i - 24)*0x400)) == pMemory){
                uint8_t j = i;
                while(systemmap.ownership[i]){
                    systemmap.sectors &= ~(mask << j);
                    j++;
                    bigallocated--;
                    systemmap.ownership[i]--;
                }
                i = 40;
            }
        }
        else{
            if(((void *)(0x20001000 + i*0x200) == pMemory)){
                uint8_t j = i;
                while(systemmap.ownership[i]){
                    systemmap.sectors &= ~(mask << i);
                    j++;
                    littleallocated--;
                    systemmap.ownership[i]--;
                }
                i = 40;
                }
            }
        }
}

// REQUIRED: include your solution from the mini project
void allowFlashAccess(void)
{
    NVIC_MPU_NUMBER_R = 6; // flash
    NVIC_MPU_BASE_R = 0;
    NVIC_MPU_ATTR_R |= (17<<1) | (3 << 24) | 1;
    NVIC_MPU_ATTR_R |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
}

void allowPeripheralAccess(void)
{
    NVIC_MPU_NUMBER_R = 7; // peripherals
    NVIC_MPU_BASE_R = 0x40000000;
    NVIC_MPU_ATTR_R |= (25<<1) | (3 << 24) | (1 << 28) | 1; // For BB and Peripherals
    NVIC_MPU_ATTR_R |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
}

void setupSramAccess(void)
{
    NVIC_MPU_NUMBER_R = 1; // 4k
    NVIC_MPU_BASE_R = (0x20001000); // Base Address
    NVIC_MPU_ATTR_R |= (11<<1) | (3 << 24) | (1 << 28) | 1; // RWX, deny execution
    NVIC_MPU_ATTR_R |= 0x0000FF00; // Disable SRD bits

    NVIC_MPU_NUMBER_R = 2; // 4k
    NVIC_MPU_BASE_R = (0x20002000);
    NVIC_MPU_ATTR_R |= (11<<1) | (3 << 24) | (1 << 28) | 1;
    NVIC_MPU_ATTR_R |= 0x0000FF00;

    NVIC_MPU_NUMBER_R = 3; // 4k
    NVIC_MPU_BASE_R = (0x20003000);
    NVIC_MPU_ATTR_R |= (11<<1) | (3 << 24) | (1 << 28) | 1;
    NVIC_MPU_ATTR_R |= 0x0000FF00;

    NVIC_MPU_NUMBER_R = 4; // 8k
    NVIC_MPU_BASE_R = (0x20004000);
    NVIC_MPU_ATTR_R |= (12<<1) | (3 << 24) | (1 << 28) | 1;
    NVIC_MPU_ATTR_R |= 0x0000FF00;

    NVIC_MPU_NUMBER_R = 5; // 8k
    NVIC_MPU_BASE_R = (0x20006000);
    NVIC_MPU_ATTR_R = (12<<1) | (3 << 24) | (1 << 28) | 1;
    NVIC_MPU_ATTR_R |= 0x0000FF00;
}

uint64_t createNoSramAccessMask(void)
{
    return 0xFFFFFFFFFFFFFFFF;
}

void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
    *srdBitMask = ~systemmap.mask; //All I need to do, generation is taken care of in malloc during the final allocation to the system table.
}

void applySramAccessMask(uint64_t srdBitMask)
{
    uint32_t realMask = 0;

    NVIC_MPU_NUMBER_R = 1;
    realMask = srdBitMask;
    realMask <<= 8;
    realMask &= 0x0000FF00;
    realMask |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
    NVIC_MPU_ATTR_R = realMask;

    NVIC_MPU_NUMBER_R = 2;
    realMask &= 0x0000FF00;
    realMask |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
    NVIC_MPU_ATTR_R = realMask;

    NVIC_MPU_NUMBER_R = 3;
    realMask = srdBitMask >> 8;
    realMask &= 0x0000FF00;
    realMask |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
    NVIC_MPU_ATTR_R = realMask;

    NVIC_MPU_NUMBER_R = 4;
    realMask = srdBitMask >> 16;
    realMask &= 0x0000FF00;
    realMask |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
    NVIC_MPU_ATTR_R = realMask;

    NVIC_MPU_NUMBER_R = 5;
    realMask = srdBitMask >> 24;
    realMask &= 0x0000FF00;
    realMask |= (NVIC_MPU_ATTR_R & 0xFFFF00FF);
    NVIC_MPU_ATTR_R = realMask;

}


