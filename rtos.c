// RTOS Framework - Fall 2024
// J Losh

// Student Name:Jay Bhatt
// ID number 1002173351

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// 6 Pushbuttons and 5 LEDs, UART
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1
// Memory Protection Unit (MPU):
//   Region to control access to flash, peripherals, and bitbanded areas
//   4 or more regions to allow SRAM access (RW or none for task)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "tm4c123gh6pm.h"      // Device-specific header file
#include "clock.h"            // System clock configuration functions
#include "gpio.h"             // GPIO pin control functions
#include "uart0.h"            // UART0 communication interface
#include "wait.h"             // Delay functions
#include "mm.h"               // Memory management functions
#include "kernel.h"           // Kernel and RTOS-related functions
#include "faults.h"           // Fault handling mechanisms
#include "tasks.h"            // Task creation and management functions
#include "shell.h"            // Shell interface for user commands



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    bool ok;                         // Boolean to track successful initialization

    // Initialize hardware components and peripherals
    initSystemClockTo40Mhz();
    initHw();
    initTimer();
    initUart0();
    initFaultInterrupts();

    // Setup Memory Protection Unit (MPU) for memory access control
    BackgroundRules();
    allowFlashAccess();
    allowPeripheralAccess();
    setupSramAccess();

    // Enable MPU
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE;
    initRtos();


    // Configure UART0 communication baud rate
    setUart0BaudRate(115200, 40e6);


    // Initialize mutexes and semaphores for synchronization
    initMutex(resource);
    initSemaphore(keyPressed, 1);
    initSemaphore(keyReleased, 0);
    initSemaphore(flashReq, 5);


    // Add the idle task (mandatory for RTOS) with the lowest priority
        ok =  createThread(idle, "Idle", 15, 512);

       
    // Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 12, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 8, 512);
    ok &= createThread(oneshot, "OneShot", 4, 1536);
    ok &= createThread(readKeys, "ReadKeys", 12, 1024);
    ok &= createThread(debounce, "Debounce", 12, 1024);
    ok &= createThread(important, "Important", 0, 1024);
    ok &= createThread(uncooperative, "Uncoop", 12, 1024);
    ok &= createThread(errant, "Errant", 12, 512);
    ok &= createThread(shell, "Shell", 12, 4096);

   // Start the RTOS (only if all tasks are successfully created)
    if (ok)
        startRtos(); // never returns
    else
        while(true);        // Infinite loop in case of initialization failure
}
