///*
// * turbidity.c
// *
// *  Created on: Sep 19, 2025
// *      Author: Dylan Nguyen
// *
// *  SEN0189 Turbidity sensor on TM4C123GXL:
// *   - Analog input on PE0 (AIN3) -> ADC0 SS3
// *   - RGB LED on PF1 (R), PF2 (B), PF3 (G) used for heartbeat/range indication
// *
// *  NOTE: If the sensor is powered at 5V, you MUST use a voltage divider so that ADC pin never exceeds 3.3V.
// *        If you power the sensor from 3.3V, you can connect AO directly to PE0 and set the configuration below.
// *        Update R_TOP_OHMS and R_BOTTOM_OHMS to match your hardware if you keep the divider.
// */
//#include <stdint.h>
//#include <stdbool.h>
//#include "../kernel/kernel.h"     // sleep()
//#include "../dependencies/wait.h" // waitMicrosecond()
//#include "../drivers/uart0.h"     // putsUart0()
//#include "../dependencies/tm4c123gh6pm.h"
//#include "turbidity.h"
//
//#ifndef putuart
//#define putuart putsUart0
//#endif
//
//// ---------------- Configuration ----------------
//
//// ADC channel selection: PE0 is AIN3 (default wiring)
//#define TURBIDITY_ADC_CH           3U           // AIN3 index
//#define TURBIDITY_ADC_CH_MASK      (0xFu)       // mask for SSMUX3 field
//
//// ADC reference (VDDA)
//#define VREF_ADC                   3.3f
//#define ADC_MAX_COUNTS             4095.0f
//
//// Sensor supply and scaling configuration
//// If powering SEN0189 from 3.3V: set SENSOR_SUPPLY_VOLTS to 3.3f and USE_DIVIDER to 0 (direct AO->PE0)
//// If powering SEN0189 from 5.0V: set SENSOR_SUPPLY_VOLTS to 5.0f and USE_DIVIDER to 1 (with divider values below)
//#define SENSOR_SUPPLY_VOLTS        3.3f   // 3.3f or 5.0f
//#define USE_DIVIDER                0      // 0 = direct, 1 = undo divider
//
//// External voltage divider used to drop sensor's ~0..4.5V to 0..3.3V (only when USE_DIVIDER == 1)
//// V_adc = V_sensor * (R_BOTTOM / (R_TOP + R_BOTTOM))  =>  V_sensor = V_adc * (R_TOP + R_BOTTOM) / R_BOTTOM
//// Update these to your actual resistor values (ohms)
//#define R_TOP_OHMS                 3300.0f
//#define R_BOTTOM_OHMS              9100.0f
//
//// DFRobot polynomial (approx) for SEN0189 on 5V system: NTU = a2*V^2 + a1*V + a0
//#define NTU_A2                     (-1120.4f)
//#define NTU_A1                     (5742.3f)
//#define NTU_A0                     (-4352.9f)
//
//// LED pins (TM4C123GXL on-board)
//#define LED_PORT_BASE              GPIO_PORTF_BASE
//#define LED_R                      (1U << 1) // PF1
//#define LED_B                      (1U << 2) // PF2
//#define LED_G                      (1U << 3) // PF3
//#define LED_MASK                   (LED_R | LED_G | LED_B)
//
//// Sampling cadence
//#define SAMPLE_PERIOD_MS           100U     // 10 Hz
//#define SETTLE_US                  100U     // settle time before sampling
//#define AVG_SAMPLES                16U      // additional software averaging
//
//// NTU thresholds for LED coloring
//#define NTU_LOW                    50.0f
//#define NTU_HIGH                   150.0f
//
//// ---------------- Local Helpers ----------------
//
//static void initLeds(void)
//{
//    // Enable GPIOF clock
//    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;   // Port F
//    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0) { }
//
//    // Configure PF1, PF2, PF3 as digital outputs
//    GPIO_PORTF_DIR_R |= LED_MASK;
//    GPIO_PORTF_DEN_R |= LED_MASK;
//    // Turn all off
//    GPIO_PORTF_DATA_R &= ~LED_MASK;
//}
//
//static inline void ledsOff(void)
//{
//    GPIO_PORTF_DATA_R &= ~LED_MASK;
//}
//
//static inline void ledSet(uint8_t pins)
//{
//    // Only affect RGB pins
//    uint32_t data = GPIO_PORTF_DATA_R;
//    data &= ~LED_MASK;
//    data |= (pins & LED_MASK);
//    GPIO_PORTF_DATA_R = data;
//}
//
//static inline void ledOn(uint8_t pins)
//{
//    GPIO_PORTF_DATA_R |= (pins & LED_MASK);
//}
//
//static inline void ledOff(uint8_t pins)
//{
//    GPIO_PORTF_DATA_R &= ~(pins & LED_MASK);
//}
//
//static inline void ledToggle(uint8_t pins)
//{
//    GPIO_PORTF_DATA_R ^= (pins & LED_MASK);
//}
//
//static void initAdcAin3(void)
//{
//    // Enable clocks
//    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; // Port E
//    SYSCTL_RCGCADC_R  |= SYSCTL_RCGCADC_R0;  // ADC0
//    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4) == 0) { }
//    while ((SYSCTL_PRADC_R  & SYSCTL_PRADC_R0)  == 0) { }
//
//    // PE0 -> AIN3 analog input
//    GPIO_PORTE_AFSEL_R |= (1U << 0);  // alt func
//    GPIO_PORTE_DEN_R   &= ~(1U << 0); // digital disable
//    GPIO_PORTE_AMSEL_R |= (1U << 0);  // analog enable
//
//    // ADC configuration
//    ADC0_PC_R      = 0x1;       // 125 ksps
//    ADC0_SSPRI_R   = 0x3210;    // SS3 highest priority
//    ADC0_SAC_R     = 0x6;       // 64x hardware averaging for noise reduction
//
//    ADC0_ACTSS_R  &= ~ADC_ACTSS_ASEN3;                // disable SS3 for config
//    ADC0_EMUX_R   &= ~ADC_EMUX_EM3_M;                 // SS3 trigger = processor (software)
//    ADC0_SSMUX3_R  = (ADC0_SSMUX3_R & ~TURBIDITY_ADC_CH_MASK) | TURBIDITY_ADC_CH; // AIN3
//    ADC0_SSCTL3_R  = ADC_SSCTL3_END0 | ADC_SSCTL3_IE0; // single sample, end, interrupt enable
//    ADC0_IM_R     &= ~ADC_IM_MASK3;                   // no NVIC interrupt (polling)
//    ADC0_ISC_R     = ADC_ISC_IN3;                     // clear any prior flag
//    ADC0_ACTSS_R  |= ADC_ACTSS_ASEN3;                 // enable SS3
//}
//
//// Print unsigned integer via UART without stdio (C89-safe)
//static void putUInt(uint32_t v)
//{
//    char buf[11];
//    int  i = 0;
//    if (v == 0)
//    {
//        buf[i++] = '0';
//    }
//    else
//    {
//        char rev[11];
//        int  j = 0;
//        while (v != 0 && j < (int)sizeof(rev))
//        {
//            rev[j++] = (char)('0' + (v % 10));
//            v /= 10;
//        }
//        while (j > 0)
//        {
//            buf[i++] = rev[--j];
//        }
//    }
//    buf[i] = '\0';
//    putuart(buf);
//}
//
//// ---------------- Public API ----------------
//
//void initTurbidity(void)
//{
//    initLeds();
//    initAdcAin3();
//}
//
//uint16_t readTurbidityRaw(void)
//{
//    ADC0_PSSI_R = ADC_PSSI_SS3;                 // start conversion
//    while ((ADC0_RIS_R & ADC_RIS_INR3) == 0) { } // wait complete
//    uint16_t result = (uint16_t)(ADC0_SSFIFO3_R & 0x0FFF);
//    ADC0_ISC_R = ADC_ISC_IN3;                   // clear flag
//    return result;
//}
//
//uint16_t readTurbidityAverage(uint16_t samples)
//{
//    uint16_t n;
//    uint32_t acc;
//    uint16_t i;
//    if (samples == 0)
//        n = 1;
//    else
//        n = samples;
//    acc = 0;
//    for (i = 0; i < n; i++)
//    {
//        acc += readTurbidityRaw();
//    }
//    return (uint16_t)(acc / (uint32_t)n);
//}
//
//float turbidityCountsToAdcVolts(uint16_t counts)
//{
//    return ((float)counts) * (VREF_ADC / ADC_MAX_COUNTS);
//}
//
//float turbidityAdcVoltsToSensorVolts(float v_adc)
//{
//    float v;
//#if USE_DIVIDER
//    // Undo divider: V_sensor = V_adc * (R_top + R_bottom) / R_bottom
//    v = v_adc * ((R_TOP_OHMS + R_BOTTOM_OHMS) / R_BOTTOM_OHMS);
//#else
//    // No divider: sensor voltage equals ADC pin voltage
//    v = v_adc;
//#endif
//    if (v < 0.0f) v = 0.0f;
//    if (v > SENSOR_SUPPLY_VOLTS) v = SENSOR_SUPPLY_VOLTS; // clamp to sensor supply
//    return v;
//}
//
//float turbidityVoltsToNTU(float v_sensor)
//{
//    // Polynomial is characterized for ~5V supply; scale if using 3.3V
//    float v_for_poly = v_sensor;
//    if (SENSOR_SUPPLY_VOLTS < 4.9f)
//    {
//        v_for_poly = v_sensor * (5.0f / SENSOR_SUPPLY_VOLTS);
//        if (v_for_poly > 4.5f) v_for_poly = 4.5f; // stay within typical curve domain
//    }
//    float ntu = (NTU_A2 * v_for_poly * v_for_poly) + (NTU_A1 * v_for_poly) + NTU_A0;
//    if (ntu < 0.0f) ntu = 0.0f;
//    return ntu;
//}
//
//void turbidityTask(void)
//{
//    initTurbidity();
//
//    // Start with LEDs off
//    ledsOff();
//
//    while (true)
//    {
//        // Small settle time for ADC mux/external RC
//        waitMicrosecond(SETTLE_US);
//
//        // Read with extra software averaging on top of hardware averaging
//        uint16_t raw = readTurbidityAverage(AVG_SAMPLES);
//
//        // Convert to voltages and NTU
//        float v_adc    = turbidityCountsToAdcVolts(raw);
//        float v_sensor = turbidityAdcVoltsToSensorVolts(v_adc);
//        float ntu_f    = turbidityVoltsToNTU(v_sensor);
//
//        // Integer-friendly prints
//        uint32_t mv_sensor = (uint32_t)(v_sensor * 1000.0f + 0.5f);
//        uint32_t ntu_i     = (uint32_t)(ntu_f + 0.5f);
//
//        // UART status line without mixed declarations
//        putuart("TURB: raw=");
//        putUInt((uint32_t)raw);
//        putuart(", Vs=");
//        putUInt(mv_sensor);
//        putuart(" mV, NTU=");
//        putUInt(ntu_i);
//        putuart("\r\n");
//
//        // Heartbeat: toggle blue every sample
//        ledToggle(LED_B);
//
//        // Range indication via red/green (keep blue heartbeat independent)
//        if (ntu_f < NTU_LOW)
//        {
//            // Low turbidity: green on, red off
//            ledOn(LED_G);
//            ledOff(LED_R);
//        }
//        else if (ntu_f < NTU_HIGH)
//        {
//            // Medium: both red and green off (only heartbeat blue visible)
//            ledOff(LED_G | LED_R);
//        }
//        else
//        {
//            // High turbidity: red on, green off
//            ledOn(LED_R);
//            ledOff(LED_G);
//        }
//
//        // Sleep until next period
//        sleep(SAMPLE_PERIOD_MS);
//    }
//}


/*
 * turbidity.c
 *
 *  Created on: Sep 19, 2025
 *      Author: Dylan Nguyen
 */
#include <stdint.h>
#include <stdbool.h>
#include "../kernel/kernel.h"     // sleep()
#include "../dependencies/wait.h" // waitMicrosecond()
#include "../drivers/uart0.h"     // putsUart0()
#include "../drivers/gpio.h"      // enablePort(), selectPinAnalogInput()
#include "../dependencies/tm4c123gh6pm.h"

#ifndef putuart
#define putuart putsUart0
#endif

// Initialization code for turbidity sensor (DFRobot SEN0189) on PE0 (AIN3)
void initTurbidity(void)
{
    // Use GPIO helper to init Port E and configure PE0 as analog input
    enablePort(PORTE);
    selectPinAnalogInput(PORTE, 0); // PE0 -> AIN3

    // Enable ADC0 clock and wait ready
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0; // ADC0 clock
    while ((SYSCTL_PRADC_R & SYSCTL_PRADC_R0) == 0);

    // ADC0 setup for single sample on SS3, software trigger, AIN3 (PE0)
    ADC0_PC_R = 0x01;      // 125 ksps
    ADC0_SSPRI_R = 0x3210; // SS3 highest priority

    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3; // disable SS3 during config
    ADC0_EMUX_R = (ADC0_EMUX_R & ~ADC_EMUX_EM3_M) | ADC_EMUX_EM3_PROCESSOR; // Select software (processor) trigger for SS3
    ADC0_SSMUX3_R = 3;                                // AIN3 -> PE0
    ADC0_SSCTL3_R = ADC_SSCTL3_END0 | ADC_SSCTL3_IE0; // single sample, EoS
    ADC0_IM_R &= ~ADC_IM_MASK3;                       // no interrupt (polling)
    ADC0_ISC_R = ADC_ISC_IN3;                         // clear any prior flag
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                  // enable SS3
}

// Read one 12-bit sample from ADC0 SS3 (PE0/AIN3)
uint16_t readTurbidityRaw(void)
{
    ADC0_PSSI_R = ADC_PSSI_SS3;              // start conversion
    while ((ADC0_RIS_R & ADC_RIS_INR3) == 0); // wait complete
    uint16_t result = (uint16_t)(ADC0_SSFIFO3_R & 0x0FFF);
    ADC0_ISC_R = ADC_ISC_IN3; // clear flag
    return result;
}

void turbidityTask(void)
{
    initTurbidity();

    while (true)
    {
        // Debug output
        putsUart0("Analog Value: ");

        // Read raw ADC (unused for now)
        putiUart0((uint32_t)readTurbidityRaw());
        putcUart0("\n");
        // Sleep for 1000 ms
        sleep(1000);
    }
}





