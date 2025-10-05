///*
// * rtc.c
// *
// *  Created on: Oct 2, 2025
// *      Author: clara
// */
//
//#include "rtc.hpp"
//#include "../gpio.h"
//
//RealTimeClock::RealTimeClock()
//{
//    TIMER0_CTL_R &= ~(TIMER_CTL_TAEN);                 // turn-off timer before reconfiguring
//    TIMER0_CFG_R = TIMER_CFG_32_BIT_RTC;           // configure as 32-bit timer (A+B)
//    TIMER0_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
//}
//
//RealTimeClock::getTimeSeconds()
//{
//    return TIMER0_TAV_R;
//}
//
//
