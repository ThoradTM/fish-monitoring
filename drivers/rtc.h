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
} RTCDateTime;

/* Function Prototypes */
void RTCInit(void);
void RTCSetDateTime(const RTCDateTime *dt);
void RTCGetDateTime(RTCDateTime *dt);

uint32_t RTCGetSeconds(void);

#endif /* RTC_H */

