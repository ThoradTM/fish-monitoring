#include "temperature.h"
#include "../kernel/kernel.h"
#include "../drivers/pain.h"
#include "../dependencies/wait.h"
#include "../drivers/gpio.h"


void tempInit()
{
    enablePort(PORTC);
    selectPinPushPullOutput(PORTC, 7);
    setHigh();
}

inline setHigh() { setPinValue(PORTC, 7, 1); }

inline setLow() { setPinValue(PORTC, 7, 0); }

inline writeOne()
{
    setLow();
    waitMicrosecond(12);
    setHigh();
    waitMicrosecond(48);
}

inline writeZero()
{
    setLow();
    waitMicrosecond(60);
    setHigh();
    waitMicrosecond(1);
}

inline uint8_t readDigit()
{
    uint8_t retval;
    waitMicrosecond(30);
    retval = getPinValue(PORTC, 7);
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
    uint8_t retval;
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        retval |= readDigit();
        retval << 1;
    }
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

    if(timer > 1)
    {
        putsUart0("ERROR: Init failed! Sensor not found!");
        putcUart0('\n');
    }
    else
    {
        uint8_t regNum = 0;
        uint8_t readBuf[9];
        while(1)
        {
            GREEN_OB_LED ^= 1;
            sleep(500);

            while(regNum < 9 )
            {
                uint8_t writeBuf = 0xBE;

                writeTemp8(writeBuf);

                selectPinDigitalInput(PORTC, 7);

                readBuf[regNum] = readTemp8();

                selectPinPushPullOutput(PORTC, 7);
                setHigh();
                regNum++;
            }
            regNum = 0;

            uint8_t i;
            for(i = 0; i < 9; i++)
            {
                puthUart0(readBuf[i]);
                putcUart0('\n');
            }

        }
    }
}
