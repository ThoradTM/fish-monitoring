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

// uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
// {
//     uint8_t crc = 0;
//     for (uint8_t i = 0; i < len; i++)
//     {
//         uint8_t inbyte = data[i];
//         for (uint8_t j = 0; j < 8; j++)
//         {
//             uint8_t mix = (crc ^ inbyte) & 0x01;
//             crc >>= 1;
//             if (mix)
//                 crc ^= 0x8C;  // Dallas/Maxim polynomial
//             inbyte >>= 1;
//         }
//     }
//     return crc;
// }


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
        uint8_t regNum = 0;
        uint8_t i = 0;

        while(1)
        {

             sleep(1000);  // 1 second delay between readings
    
            // 1. Start conversion
            startSequence();
            writeTemp8(SKIP_ROM);
            writeTemp8(INIT_CONVERT);

            uint8_t obvs = 0;
           while(!obvs)
           {
//         Waiting on conversion to be done
                obvs = readDigit();
           }

            // 2. Read scratchpad
            startSequence();
            writeTemp8(SKIP_ROM);
            writeTemp8(READ_SCRATCHPAD);
            
            uint8_t readBuf[9];
            for(regNum = 0; regNum < 9; regNum++)
            {
                readBuf[regNum] = readTemp8();
            }
            
            // 3. Print raw data
            putsUart0("Scratchpad read\n");
            for(i = 0; i < 9; i++)
            {
                puthUart0(readBuf[i]);
                putcUart0('\n');
            }
            
            // 4. Calculate temperature
            int16_t temp_raw = (readBuf[1] << 8) | readBuf[0];
            int32_t temperature = (temp_raw * 100) / 16;
            
            putsUart0("Temperature: ");
            putiUart0((int)temperature);
            putsUart0(" C\n");
        }
//            //GREEN_OB_LED ^= 1;
//            waitMicrosecond(1000000);

//            writeTemp8(SKIP_ROM);

//            writeTemp8(INIT_CONVERT);


//             waitMicrosecond(750000);  // Wait 750ms, not 30μs!


//             uint8_t obvs = 0;
//            while(!obvs)
//            {
// //                            Waiting on conversion to be done
//                 obvs = readDigit();
//             //    putiUart0(obvs);
//            }

//            startSequence();



//            writeTemp8(SKIP_ROM);

//            writeTemp8(READ_SCRATCHPAD);

//            for(regNum = 0; regNum < 9; regNum++)
//            {
//                readBuf[regNum] = readTemp8();
//            }



//            putsUart0("Scratchpad read");
//            putcUart0('\n');
//            for(i = 0; i < 9; i++)
//            {
//                puthUart0(readBuf[i]);
//                putcUart0('\n');
//            }



        //    uint8_t Temp_LSB = readTemp8();
        //    uint8_t Temp_MSB = readTemp8();
        //    int Temp = ((Temp_MSB<<8))|Temp_LSB;
        //    float temperature = (float)Temp/16.0;
        //    putiUart0(Temp/16);

        //    putcUart0('\n');




            //startSequence();

        //     uint8_t i = 5;
        //    for(i = 100; i > 0; i--)
        //    {


        //    }
        //    sleep(500);






        }
    
    shmPerms();
    
    shm * sharedSpace = getShmHandle();
    while(1)
    {
        sleep(1000);
        
        lock(resource);
        sharedSpace->temperature++;
        unlock(resource);

       sharedSpace->shared = sharedSpace->shared + 1;

       puthUart0(sharedSpace->shared);
       putcUart0('\n');

    } // Fault state. Tasks do not close gracefully at the close of a function.
}

