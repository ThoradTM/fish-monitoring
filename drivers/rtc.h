// RTC EXAMPLE. FOR REFERENCE USE ONLY.


/* rtc.h - RTC Driver Header for TM4C123GH6PM */
#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <stdbool.h>

/* RTC Date/Time Structure */
typedef struct {
    uint8_t seconds;    // 0-59
    uint8_t minutes;    // 0-59
    uint8_t hours;      // 0-23
    uint8_t day;        // 1-31
    uint8_t month;      // 1-12
    uint16_t year;      // Full year (e.g., 2025)
} RTC_DateTime;

/* Function Prototypes */
void RTC_Init(void);
void RTC_SetDateTime(const RTC_DateTime *dt);
void RTC_GetDateTime(RTC_DateTime *dt);

uint32_t RTC_GetSeconds(void);
void RTC_SetAlarm(uint32_t seconds);
void RTC_EnableAlarmInterrupt(void);
void RTC_DisableAlarmInterrupt(void);

#endif /* RTC_H */
