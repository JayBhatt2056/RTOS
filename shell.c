// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------
#include <stdbool.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "shell.h"
#include "string.h"
#include "kernel.h"
#include "string.h"
#include "tasks.h"
#include "gpio.h"
#include "wait.h"







#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTC,6 // off-board red LED
#define ORANGE_LED PORTC,7 // off-board orange LED
#define YELLOW_LED PORTC,4 // off-board yellow LED
#define GREEN_LED  PORTC,5 // off-board green LED



PSInfo psInfo;



// REQUIRED: Add header files here for your strings functions, ...

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void ps(PSInfo* info)
{
    __asm(" SVC #18");

}
void ipcs(IPCSInfo* info)
{
    __asm(" SVC #14");

}
void kill(uint32_t pid)
{
    __asm(" SVC #9");
}

void pi(bool p)
{
    if(p == true)
    {
        putsUart0("pi on");
    }
    if (p == false)
    {
        putsUart0("pi off");
    }
}

void preempt(bool p)
{
    __asm(" SVC #6");
}


void sched(bool p)
{
    __asm(" SVC #7");
}

void pkill(char* proc_name)
{
    __asm(" SVC #8");
}

void pidof(char* name)
{
    __asm(" SVC #10");
}
void proc(char* name)
{
    __asm(" SVC #15");
}
void Reboot(void)
{
    __asm(" SVC #13");
}


// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA data;


    while(true){
        if (kbhitUart0() == 1)
        {
            getsUart0(&data);
            parseFields(&data);

            if(isCommand(&data,"reboot",0))
            {
                Reboot();


                //putsUart0("reboot");
            }
            if(isCommand(&data,"ps",0))
            {
                uint8_t i;
                PSInfo psInfo;
                ps(&psInfo);

                // Display process details
                putsUart0("PID     Name            State   CPU%    Blocking Resource\r\n");
                for (i = 0; i < psInfo.taskCount; i++)
                {
                    char buffer[16];
                    itoa(psInfo.tasks[i].pid, buffer);
                    putsUart0(buffer);
                    putsUart0("    ");

                    putsUart0(psInfo.tasks[i].name);
                    putsUart0("    ");

                    itoa(psInfo.tasks[i].state, buffer);
                    putsUart0(buffer);
                    putsUart0("    ");

                    itoa(psInfo.tasks[i].cpuPercent, buffer);
                    putsUart0(buffer);
                    putsUart0("%    ");

                    if (psInfo.tasks[i].blockingResourceType == 1)
                    {
                        putsUart0("Mutex ");
                        itoa(psInfo.tasks[i].blockingResourceId, buffer);
                        putsUart0(buffer);
                    }
                    else if (psInfo.tasks[i].blockingResourceType == 2)
                    {
                        putsUart0("Semaphore ");
                        itoa(psInfo.tasks[i].blockingResourceId, buffer);
                        putsUart0(buffer);
                    }
                    else
                    {
                        putsUart0("None");
                    }
                    putsUart0("\r\n");
                }

            }
            if(isCommand(&data,"ipcs",0))
            {
                uint8_t i,j;
                IPCSInfo ipcsInfo;

                ipcs(&ipcsInfo);
                char numStr[12];  // Buffer for integer-to-string conversions

                // Display Mutex Information
                putsUart0("Mutexes:\r\n");
                for (i = 0; i < SHELL_MAX_MUTEXES; i++)
                {
                    putsUart0("Mutex ");
                    itoa(i, numStr);             // Convert index to string
                    putsUart0(numStr);
                    putsUart0(": Locked=");
                    itoa(ipcsInfo.mutexes[i].lock, numStr); // Convert lock status to string
                    putsUart0(numStr);
                    putsUart0(", QueueSize=");
                    itoa(ipcsInfo.mutexes[i].queueSize, numStr); // Convert queue size to string
                    putsUart0(numStr);
                    putsUart0(", LockedBy=");
                    itoa(ipcsInfo.mutexes[i].lockedBy, numStr);  // Convert lockedBy to string
                    putsUart0(numStr);
                    putsUart0("\r\nQueue: [");

                    // Display queue contents
                    for (j = 0; j < SHELL_MAX_MUTEX_QUEUE_SIZE; j++)
                    {
                        if (j > 0)
                            putsUart0(", ");
                        itoa(ipcsInfo.mutexes[i].processQueue[j], numStr);
                        putsUart0(numStr);
                    }
                    putsUart0("]\r\n");
                }

                // Display Semaphore Information
                putsUart0("Semaphores:\r\n");
                for (i = 0; i < SHELL_MAX_SEMAPHORES; i++)
                {
                    putsUart0("Semaphore ");
                    itoa(i, numStr);             // Convert index to string
                    putsUart0(numStr);
                    putsUart0(": Count=");
                    itoa(ipcsInfo.semaphores[i].count, numStr);  // Convert count to string
                    putsUart0(numStr);
                    putsUart0(", QueueSize=");
                    itoa(ipcsInfo.semaphores[i].queueSize, numStr); // Convert queue size to string
                    putsUart0(numStr);
                    putsUart0("\r\nQueue: [");

                    // Display queue contents
                    for (j = 0; j < SHELL_MAX_SEMAPHORE_QUEUE_SIZE; j++)
                    {
                        if (j > 0)
                            putsUart0(", ");
                        itoa(ipcsInfo.semaphores[i].processQueue[j], numStr);
                        putsUart0(numStr);
                    }
                    putsUart0("]\r\n");
                }
            }
            if(isCommand(&data,"kill",1))
            {

                uint32_t pid = getFieldInteger(&data,1);
                kill(pid);


            }
            if(isCommand(&data,"pkill",1))
            {
                char* proc_name = getFieldString(&data, 1);
                pkill(proc_name);

            }

            if(isCommand(&data,"preempt",1))
            {
                char* status = getFieldString(&data, 1);
                if (compare_string(status, "on"))
                {

                    preempt(true);

                }
                if(compare_string(status, "off"))
                {
                    preempt(false);
                }

            }
            if(isCommand(&data,"sched",1))
            {
                char* status = getFieldString(&data, 1);
                if (compare_string(status, "prio"))
                {

                    sched(true);

                }
                if(compare_string(status, "rr"))
                {
                    sched(false);
                }

            }
            if(isCommand(&data,"Pidof",1))
            {
                char* name = getFieldString(&data, 1);
                pidof(name);

            }

           char* name = getFieldString(&data, 0);
           if(name != 0)
           {
           proc(name);
           }
            }
        yield();
        }


    }

