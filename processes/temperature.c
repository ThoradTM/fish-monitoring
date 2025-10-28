#include "temperature.h"
#include "../kernel/kernel.h"
#include "../drivers/pain.h"
#include "../libraries/wait.h"
#include "../drivers/gpio.h"
#include "../kernel/shm.h"
#include "../drivers/uart0.h"
#include "../kernel/servicecalls.h"


// PIN PC7 AS COMM LINE

inline setHigh() { setPinValue(PORTC, 7, 1); }

inline setLow() { setPinValue(PORTC, 7, 0); }


void tempInit()
{
    enablePort(PORTC);
    selectPinOpenDrainOutput(PORTC, 7);
    setHigh();
    waitMicrosecond(2);
}

// For later comparison

// inline writeOne()
// {
//     setLow();
//     waitMicrosecond(8);
//     setHigh();
//     waitMicrosecond(62);
//     waitMicrosecond(2);
// }

// inline writeZero()
// {
//     setLow();
//     waitMicrosecond(70);
//     setHigh();
//     waitMicrosecond(2);
// }

inline writeOne()
{
    setLow();
    waitMicrosecond(6);   // 6μs low
    setHigh();
    waitMicrosecond(64);  // 64μs high (total 70μs slot)
}

inline writeZero()
{
    setLow();
    waitMicrosecond(60);  // 60μs low
    setHigh();
    waitMicrosecond(10);  // 10μs recovery (total 70μs slot)
}

// For later comparison

// inline uint8_t readDigit()
// {
//     uint8_t retval = 0;
//     setLow();
//     waitMicrosecond(5);
//     setHigh();

//     selectPinDigitalInput(PORTC, 7);

//     waitMicrosecond(15);
//     if(getPinValue(PORTC, 7)) retval = 0x01;

//     selectPinOpenDrainOutput(PORTC, 7);

//     waitMicrosecond(50);

//     waitMicrosecond(2);
//     return retval;
// }

inline uint8_t readDigit()
{
    uint8_t retval;
    
    // Initiate read slot
    setLow();
    waitMicrosecond(3);  // Pull low for 3μs
    
    // Release and switch to input
    selectPinDigitalInput(PORTC, 7);
    
    // Wait then sample (must sample within 15μs of falling edge)
    waitMicrosecond(12);  // Wait 12μs total from start
    retval = getPinValue(PORTC, 7);  // Sample the bit
    
    // Switch back to output and wait for recovery
    selectPinOpenDrainOutput(PORTC, 7);
    setHigh();
    
    waitMicrosecond(60);  // Wait for slot to complete (70μs total minimum)
    
    return retval;
}

inline writeTemp8(uint8_t buf)
{
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        ((buf >> i) & 1) ? writeOne() : writeZero();
    }
}

inline uint8_t readTemp8()
{
    uint8_t retval = 0;
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        retval |= (readDigit() << i);
    }
    return retval;
}

inline uint8_t startSequence()
{
    setLow();
    waitMicrosecond(480);
    setHigh();

    uint8_t timer = 0;

    selectPinDigitalInput(PORTC, 7);

    waitMicrosecond(70);

    // Presence pulse: device pulls low
    timer = getPinValue(PORTC, 7);

    // Finish reset slot (~480 µs total)
    waitMicrosecond(410);

    // Return bus to open-drain output
    selectPinOpenDrainOutput(PORTC, 7);

    return timer;
}

#define READ_FULL_SINGLE_DEV 0x33

#define SKIP_ROM 0xCC

#define INIT_CONVERT 0x44

#define READ_SCRATCHPAD 0xBE

void tempTask()
{
   if(startSequence() > 1)
   {
       putsUart0("ERROR: Init failed! Sensor not found!");
       putcUart0('\n');
       while(1);
   }
   else
   {
        shmPerms();
    
        shm * sharedSpace = getShmHandle();
        uint8_t regNum = 0;

        while(1)
        {

            sleep(1000);  // 1 second delay between readings
    
            startSequence();
            writeTemp8(SKIP_ROM);
            writeTemp8(INIT_CONVERT);

            uint8_t obvs = 0;
           while(!obvs)
           {
//         Waiting on conversion to be done
                obvs = readDigit();
           }
            startSequence();
            writeTemp8(SKIP_ROM);
            writeTemp8(READ_SCRATCHPAD);
            
            uint8_t readBuf[9];
            for(regNum = 0; regNum < 9; regNum++)
            {
                readBuf[regNum] = readTemp8();
            }
            
            //uint8_t i = 0;
            //putsUart0("Scratchpad read\n");
            //for(i = 0; i < 9; i++)
            //{
            //    puthUart0(readBuf[i]);
            //    putcUart0('\n');
            //}
            
            int16_t temp_raw = (readBuf[1] << 8) | readBuf[0];
            int32_t temperature = (temp_raw * 100) / 16;

            lock(resource);
            sharedSpace->temperature = temperature;
            unlock(resource);
        }
    }
}

