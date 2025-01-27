// Tasks
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"
#include "mm.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTA,2 // off-board red LED
#define ORANGE_LED PORTA,3 // off-board orange LED
#define YELLOW_LED PORTA,4 // off-board yellow LED
#define GREEN_LED  PORTE,0 // off-board green LED

#define PB0 PORTC,4
#define PB1 PORTC,5
#define PB2 PORTC,6
#define PB3 PORTC,7
#define PB4 PORTD,6
//#define PB5 PORTD,7
#define PB5 PORTF,4
#define ON 1
#define OFF 0

uint32_t start_time,end_time;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs
    enablePort(PORTF);
    enablePort(PORTA);
    enablePort(PORTE);
    enablePort(PORTC);
    enablePort(PORTD);

    selectPinDigitalInput(PB5);
    enablePinPullup(PB5);

    selectPinPushPullOutput(BLUE_LED);
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(GREEN_LED);
    // Setup pushbuttons

    selectPinDigitalInput(PB0);
    enablePinPullup(PB0);
    selectPinDigitalInput(PB1);
    enablePinPullup(PB1);
    selectPinDigitalInput(PB2);
    enablePinPullup(PB2);
    selectPinDigitalInput(PB3);
    enablePinPullup(PB3);
    selectPinDigitalInput(PB4);
    enablePinPullup(PB4);
//    selectPinDigitalInput(PB5);
//    enablePinPullup(PB5);
    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    uint8_t buttons = 0;

    // Read each pushbutton and set the corresponding bit in 'buttons'
    if (!getPinValue(PB0)) buttons = (1 << 0); // PB0 corresponds to bit 0
    if (!getPinValue(PB1)) buttons = (1 << 1); // PB1 corresponds to bit 1
    if (!getPinValue(PB2)) buttons = (1 << 2); // PB2 corresponds to bit 2
    if (!getPinValue(PB3)) buttons = (1 << 3); // PB3 corresponds to bit 3
    if (!getPinValue(PB4)) buttons = (1 << 4); // PB4 corresponds to bit 4
    if (!getPinValue(PB5)) buttons = (1 << 5); // PB5 corresponds to bit 5

    return buttons;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{

    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}
void idle2(void)
{
    while(true)
    {
        setPinValue(RED_LED, 1);
        waitMicrosecond(1000);
        setPinValue(RED_LED, 0);
        yield();
    }
}
void idle3(void)
{

    while(true)
    {
        setPinValue(BLUE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(BLUE_LED, 0);
        yield();
    }
}
void idle4(void)
{

    while(true)
    {
        setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
        sleep(125);
    }
}
void idle5(void)
{
    while(true)
    {
        setPinValue(BLUE_LED, !getPinValue(BLUE_LED));
        sleep(125);
    }
}


void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}
void SVCmallocFromHeap(uint32_t size_in_bytes, void *ptr)
{
    __asm(" SVC #16");
}
void SVCfreeToHeap(void* ptr)
{
    __asm(" SVC #17");
}
void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    void *ptr1 = 0;
    SVCmallocFromHeap(5000 * sizeof(uint8_t), (void *)&ptr1);
    mem = (uint8_t*)ptr1;
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        SVCfreeToHeap(mem);
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        sleep(1000);
        unlock(resource);
    }
}
