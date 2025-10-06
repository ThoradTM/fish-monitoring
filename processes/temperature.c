#include "temperature.h"
#include "../kernel/kernel.h"
#include "../drivers/pain.h"
#include "../libraries/wait.h"
#include "../drivers/gpio.h"
#include "../kernel/shm.h"
#include "../drivers/uart0.h"
#include "../kernel/servicecalls.h"


inline setHigh() { setPinValue(PORTC, 7, 1); }

inline setLow() { setPinValue(PORTC, 7, 0); }


void tempInit()
{
    enablePort(PORTC);
    selectPinPushPullOutput(PORTC, 7);
    setHigh();
}

inline writeOne()
{
    setLow();
    waitMicrosecond(12);
    setHigh();
    waitMicrosecond(53);
}

inline writeZero()
{
    setLow();
    waitMicrosecond(65);
    setHigh();
    waitMicrosecond(1);
}

inline uint8_t readDigit()
{
    uint8_t retval = 0;
    setLow();
    waitMicrosecond(12);
    setHigh();
    selectPinDigitalInput(PORTC, 7);
    waitMicrosecond(5);
    if(getPinValue(PORTC, 7)) retval = 0x01;
    selectPinPushPullOutput(PORTC, 7);
    waitMicrosecond(45);
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
        retval = retval << 1;
        retval |= readDigit();
    }
    return retval;
}


void tempTask()
{
    setLow();
    waitMicrosecond(480);
    setHigh();

    uint8_t timer = 0;

    selectPinDigitalInput(PORTC, 7);

    waitMicrosecond(50);
    timer += getPinValue(PORTC, 7);
    waitMicrosecond(30);
    timer += getPinValue(PORTC, 7);

    selectPinPushPullOutput(PORTC, 7);

    if(timer > 1)
    {
        putsUart0("ERROR: Init failed! Sensor not found!");
        putcUart0('\n');
    }
    else
    {
        uint8_t regNum = 0;
        uint8_t readBuf[9];
        uint8_t writeBuf = 0x33;

        writeZero();

        while(1)
        {
            GREEN_OB_LED ^= 1;
            sleep(500);

            writeTemp8(writeBuf);

            while(regNum < 9 )
            {
                readBuf[regNum] = readTemp8();
                regNum++;
            }
            regNum = 0;

            selectPinPushPullOutput(PORTC, 7);

            uint8_t i;
            putsUart0("Bulk read");
            putcUart0('\n');
            for(i = 0; i < 9; i++)
            {
                puthUart0(readBuf[i]);
                putcUart0('\n');
            }

        }
    }
    shmPerms();
    
    shm * sharedSpace = getShmHandle();
    while(1)
    {
        sleep(1000);
        
        lock(resource);
        sharedSpace->temperature++;
        unlock(resource);

//        sharedSpace->shared = sharedSpace->shared + 1;
//
//        puthUart0(sharedSpace->shared);
//        putcUart0('\n');

    } // Fault state. Tasks do not close gracefully at the close of a function.
}
