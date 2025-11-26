/* rtc.c - RTC Driver Implementation for TM4C123GH6PM */
#include "rtc.h"
#include "libraries/tm4c123gh6pm.h"

/* Hibernation Module Register Offsets */
#define HIB_RTCC        (*((volatile uint32_t *)0x400FC000))  // Counter
#define HIB_RTCM0       (*((volatile uint32_t *)0x400FC004))  // Match 0
#define HIB_RTCLD       (*((volatile uint32_t *)0x400FC00C))  // Load
#define HIB_CTL         (*((volatile uint32_t *)0x400FC010))  // Control
#define HIB_IM          (*((volatile uint32_t *)0x400FC014))  // Interrupt Mask
#define HIB_RIS         (*((volatile uint32_t *)0x400FC018))  // Raw Interrupt Status
#define HIB_MIS         (*((volatile uint32_t *)0x400FC01C))  // Masked Interrupt Status
#define HIB_IC          (*((volatile uint32_t *)0x400FC020))  // Interrupt Clear

/* Control Register Bits */
#define HIB_CTL_RTCEN   0x00000001  // RTC Counter Enable
#define HIB_CTL_CLK32EN 0x00000040  // 32.768 kHz Clock Enable
#define HIB_CTL_WRC     0x80000000  // Write Complete

/* Interrupt Mask Bits */
#define HIB_IM_RTCALT0  0x00000001  // RTC Alert 0 Interrupt Mask

/* System Control Registers */
#define SYSCTL_RCGCHIB  (*((volatile uint32_t *)0x400FE614))

/* Helper function to check if year is leap year */
static bool IsLeapYear(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* Helper function to get days in month */
static uint8_t GetDaysInMonth(uint8_t month, uint16_t year) {
    const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && IsLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

/* Convert DateTime to Unix timestamp (seconds since Jan 1, 1970) */
static uint32_t DateTimeToSeconds(const RTC_DateTime *dt) {
    uint32_t seconds = 0;
    uint16_t year;
    uint8_t month;
    
    /* Add seconds for complete years since 1970 */
    for (year = 1970; year < dt->year; year++) {
        if (IsLeapYear(year)) {
            seconds += 366 * 86400UL;
        } else {
            seconds += 365 * 86400UL;
        }
    }
    
    /* Add seconds for complete months in current year */
    for (month = 1; month < dt->month; month++) {
        seconds += GetDaysInMonth(month, dt->year) * 86400UL;
    }
    
    /* Add remaining time */
    seconds += (dt->day - 1) * 86400UL;
    seconds += dt->hours * 3600UL;
    seconds += dt->minutes * 60UL;
    seconds += dt->seconds;
    
    return seconds;
}

/* Convert Unix timestamp to DateTime */
static void SecondsToDateTime(uint32_t seconds, RTC_DateTime *dt) {
    uint32_t days = seconds / 86400UL;
    uint32_t remaining = seconds % 86400UL;
    uint16_t year = 1970;
    uint8_t month = 1;
    uint16_t daysInYear;
    
    /* Calculate time components */
    dt->seconds = remaining % 60;
    remaining /= 60;
    dt->minutes = remaining % 60;
    dt->hours = remaining / 60;
    
    /* Calculate year */
    while (1) {
        daysInYear = IsLeapYear(year) ? 366 : 365;
        if (days < daysInYear) break;
        days -= daysInYear;
        year++;
    }
    dt->year = year;
    
    /* Calculate month and day */
    while (1) {
        uint8_t daysInMonth = GetDaysInMonth(month, year);
        if (days < daysInMonth) break;
        days -= daysInMonth;
        month++;
    }
    dt->month = month;
    dt->day = days + 1;
}

/* Wait for write complete */
static void WaitForWriteComplete(void) {
    while ((HIB_CTL & HIB_CTL_WRC) == 0) {
        /* Wait */
    }
}

/* Initialize RTC */
void RTC_Init(void) {
    /* Enable Hibernation module clock */
    SYSCTL_RCGCHIB |= 0x01;
    
    /* Wait for module to be ready */
    while ((SYSCTL_RCGCHIB & 0x01) == 0) {
        /* Wait */
    }
    
    /* Enable 32.768 kHz oscillator */
    HIB_CTL |= HIB_CTL_CLK32EN;
    WaitForWriteComplete();
    
    int i;

    /* Small delay for oscillator stabilization */
    for (i = 0; i < 100000; i++);
    
    /* Enable RTC counter */
    HIB_CTL |= HIB_CTL_RTCEN;
    WaitForWriteComplete();
}

/* Set RTC date and time */
void RTC_SetDateTime(const RTC_DateTime *dt) {
    uint32_t seconds = DateTimeToSeconds(dt);
    HIB_RTCLD = seconds;
    WaitForWriteComplete();
}

/* Get current RTC date and time */
void RTC_GetDateTime(RTC_DateTime *dt) {
    uint32_t seconds = HIB_RTCC;
    SecondsToDateTime(seconds, dt);
}

/* Get raw seconds counter */
uint32_t RTC_GetSeconds(void) {
    return HIB_RTCC;
}

/* Set alarm for specified number of seconds from now */
void RTC_SetAlarm(uint32_t seconds) {
    uint32_t currentSeconds = HIB_RTCC;
    HIB_RTCM0 = currentSeconds + seconds;
    WaitForWriteComplete();
}

/* Enable RTC alarm interrupt */
void RTC_EnableAlarmInterrupt(void) {
    HIB_IM |= HIB_IM_RTCALT0;
    WaitForWriteComplete();
    
    /* Enable Hibernation interrupt in NVIC (interrupt 43) */
    NVIC_EN1_R |= (1 << (43 - 32));
}

/* Disable RTC alarm interrupt */
void RTC_DisableAlarmInterrupt(void) {
    HIB_IM &= ~HIB_IM_RTCALT0;
    WaitForWriteComplete();
}

/* Hibernation Module ISR - User should implement this */
void Hibernate_Handler(void) {
    /* Clear the interrupt */
    HIB_IC = HIB_IM_RTCALT0;
    WaitForWriteComplete();
    
    /* User code here - handle alarm event */
}

/* Example Usage:

int main(void) {
    RTC_DateTime dt;
    
    // Initialize RTC
    RTC_Init();
    
    // Set initial date/time: January 26, 2025 12:00:00
    dt.year = 2025;
    dt.month = 1;
    dt.day = 26;
    dt.hours = 12;
    dt.minutes = 0;
    dt.seconds = 0;
    RTC_SetDateTime(&dt);
    
    // Set an alarm for 10 seconds from now
    RTC_SetAlarm(10);
    RTC_EnableAlarmInterrupt();
    
    while (1) {
        // Read current time
        RTC_GetDateTime(&dt);
        
        // Your application code here
    }
}
*/
