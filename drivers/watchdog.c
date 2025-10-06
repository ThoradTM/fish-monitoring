/*
 * watchdog.c
 *
 *  Created on: Apr 19, 2024
 *      Author: clara
 */

#include "../libraries/tm4c123gh6pm.h"
#include <stdint.h>

//#define WATCHDOG0_LOAD_R        (*((volatile uint32_t *)0x40000000))
//#define WATCHDOG0_VALUE_R       (*((volatile uint32_t *)0x40000004))
//#define WATCHDOG0_CTL_R         (*((volatile uint32_t *)0x40000008))
//#define WATCHDOG0_ICR_R         (*((volatile uint32_t *)0x4000000C))
//#define WATCHDOG0_RIS_R         (*((volatile uint32_t *)0x40000010))
//#define WATCHDOG0_MIS_R         (*((volatile uint32_t *)0x40000014))
//#define WATCHDOG0_TEST_R        (*((volatile uint32_t *)0x40000418))
//#define WATCHDOG0_LOCK_R        (*((volatile uint32_t *)0x40000C00))
//
//
//#define WDT_CTL_WRC             0x80000000  // Write Complete
//#define WDT_CTL_INTTYPE         0x00000004  // Watchdog Interrupt Type
//#define WDT_CTL_RESEN           0x00000002  // Watchdog Reset Enable
//#define WDT_CTL_INTEN           0x00000001  // Watchdog Interrupt Enable
//
//#define WDT_LOCK_M              0xFFFFFFFF  // Watchdog Lock
//#define WDT_LOCK_UNLOCKED       0x00000000  // Unlocked
//#define WDT_LOCK_LOCKED         0x00000001  // Locked
//#define WDT_LOCK_UNLOCK         0x1ACCE551  // Unlocks the watchdog timer
//SYSCTL_RCGCWD_R0
//
//SYSCTL_RCGCWD_R

void initWatchdog(){
    SYSCTL_RCGCWD_R |= SYSCTL_RCGCWD_R0;
    _delay_cycles(3);
    WATCHDOG0_LOAD_R = 80000000; // 1 second;
    WATCHDOG0_CTL_R |= WDT_CTL_RESEN | WDT_CTL_INTEN;
    NVIC_EN0_R = 1 << (INT_WATCHDOG-16);
}

void reloadWatchdogIsr(){
    WATCHDOG0_LOAD_R = 80000000; // 1 second;
    WATCHDOG0_ICR_R = WDT_ICR_M;
}


