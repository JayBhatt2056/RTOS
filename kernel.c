// Kernel functions

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "Registers.h"
#include "uart0.h"
#include "shell.h"
#include "string.h"
//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks


uint32_t systemTickCount = 0;
uint32_t systemTickCount_S;
uint32_t start_time, end_time;

// control
volatile bool priorityScheduler = true;    // priority (true) or round-robin (false)
volatile bool priorityInheritance = false; // priority inheritance for mutexes
volatile bool preemption = true;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   16
struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t ticks;                // ticks until sleep complete
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
uint32_t runtime;                 // Cumulative runtime of the task (useful for profiling and scheduling decisions)
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize the Wide Timer 0
void initTimer(void)
{
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R0;    // Enable and provide clock to the timer
    _delay_cycles(3);

    WTIMER0_CTL_R       &= ~TIMER_CTL_TAEN;         // Disable timer before configuring
    WTIMER0_CFG_R       = TIMER_CFG_32_BIT_TIMER;   // Select 32-bit timer
    WTIMER0_TAMR_R      |= TIMER_TAMR_TACDIR;       // Direction = Up-counter
    WTIMER0_TAV_R        = 0;                       // Reset the timer value
}

// Initialize a mutex
bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);                // Ensure the mutex index is within valid range
    if (ok)
    {
        mutexes[mutex].lock = false;                // Initialize the mutex to an unlocked state
        mutexes[mutex].lockedBy = 0;                // Clear the identifier of the task holding the mutex
    }
    return ok;                                      // Return true if initialization succeeded, false otherwise
}
// Initialize a semaphore
bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);         // Ensure the semaphore index is within valid range
    {
        semaphores[semaphore].count = count;        // Set the semaphore's initial count
    }
    return ok;                                      // Return true if initialization succeeded, false otherwise
}



// Initialize the SysTick timer for periodic interrupts
void initSystick(void)
{
        NVIC_ST_RELOAD_R = 39999;                   // sys_clock * 1 * 10^-3 for a 1ms tick
        NVIC_ST_CURRENT_R = NVIC_ST_CURRENT_M;      // Clear current value by writing any value

        NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC;     // Enable system clock source for Systick operation
        NVIC_ST_CTRL_R |= NVIC_ST_CTRL_INTEN;       // Enable systick interrupts
        NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE;      // Start systick
}

// Initialize the RTOS, including SysTick and Task Control Blocks (TCBs)
void initRtos(void)
{
    initSystick();                                  // Initialize SysTick timer for a 1ms system timer
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;               // Mark all TCBs as invalid
        tcb[i].pid = 0;                             // Clear the process ID (PID) for each task
    }
}

// RTOS Scheduler to select the next task to run based on priority or round-robin
uint8_t rtosScheduler(void)
{
    static uint8_t lastTaskIndex = 0xFF;            // Keeps track of the last selected task for round-robin

    if (priorityScheduler)
    {
        uint8_t highestPriority = NUM_PRIORITIES;
        uint8_t selectedTask = 0xFF;
        uint8_t i;
        uint8_t count = 0;
        uint8_t candidates[MAX_TASKS];              // Store tasks with the same priority

        // Find the highest priority and collect all tasks with that priority
        for (i = 0; i < MAX_TASKS; i++)
        {
            if (tcb[i].state == STATE_READY)
            {
                if (tcb[i].currentPriority < highestPriority)
                {
                    highestPriority = tcb[i].currentPriority;
                    count = 0;                           // Reset candidate count
                }
                if (tcb[i].currentPriority == highestPriority)
                {
                    candidates[count++] = i;            // Store the task index
                }
            }
        }

        // If there are multiple tasks with the same priority, perform round-robin among them
        if (count > 0)
        {
            // Ensure lastTaskIndex is valid
            bool validLastTask = false;
            if (lastTaskIndex != 0xFF)
            {
                for (i = 0; i < count; i++)
                {
                    if (candidates[i] == lastTaskIndex && tcb[lastTaskIndex].state == STATE_READY)
                    {
                        validLastTask = true;
                        break;
                    }
                }
            }

            // If lastTaskIndex is not valid, reset it
            if (!validLastTask)
            {
                lastTaskIndex = 0xFF;
            }

            // Find the next task in the candidate list after lastTaskIndex
            uint8_t startIndex = 0;
            if (lastTaskIndex != 0xFF)
            {
                for (i = 0; i < count; i++)
                {
                    if (candidates[i] == lastTaskIndex)
                    {
                        startIndex = (i + 1) % count;           // Start from the next candidate
                        break;
                    }
                }
            }

            selectedTask = candidates[startIndex];
            lastTaskIndex = selectedTask;                       // Update the last selected task
        }

        return selectedTask;
    }
    else
    {
        // Round-robin scheduling
        bool ok;
        static uint8_t task = 0xFF;
        ok = false;
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
        return task;
    }
}

// Spawn a new thread to execute the given function
// This function switches to unprivileged mode before executing the thread function
void spawn(_fn fn)
{
    disablePrivilegedMode();                // Enable the TMPL bit in the CONTROL register
    fn();                                   // Call the method
}
// start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    static _fn fn;
    taskCurrent = rtosScheduler();
    if (tcb[taskCurrent].pid == 0)
    {

        while (1);
    }
    void *taskPID = tcb[taskCurrent].pid;
    fn = (_fn)taskPID;
    applySramAccessMask(tcb[taskCurrent].srd);          // Apply SRAM access mask based on the task's MPU configuration
    startRtosAssembly((uint32_t)(tcb[taskCurrent].sp));// Call assembly function to set up and start the RTOS
    spawn(fn);                                          // Spawn the selected task to execute its function in unprivileged mode
}


// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}

            void *ptr = mallocFromHeap(stackBytes);

            tcb[i].state = STATE_READY;
            tcb[i].pid = fn;
            tcb[i].sp = (void *)((uint32_t)ptr + stackBytes);
            tcb[i].spInit = (void *)((uint32_t)ptr + stackBytes);
            tcb[i].priority = priority;
            tcb[i].currentPriority = priority;
            tcb[i].srd = setSramAccessWindow((uint32_t *)ptr, stackBytes);

            uint8_t j;
           for (j = 0; j < 15 && name[j] != '\0'; j++)
           {
               tcb[i].name[j] = name[j];
           }
           tcb[i].name[j] = '\0';

           // Create the initial stack frame
           uint32_t *psp = (uint32_t *)tcb[i].sp;

           // Simulate processor state (top of stack frame)
           *(--psp) = 0x01000000;         // xPSR: Thumb mode
           *(--psp) = (uint32_t)tcb[i].pid;       // PC: Task entry point
           *(--psp) = 0xFFFFFFFD;         // LR: Return to Thread mode using PSP
           *(--psp) = 0xFFFFFFFF;         // R12
           *(--psp) = 0xFFFFFFFF;         // R3
           *(--psp) = 0xFFFFFFFF;         // R2
           *(--psp) = 0xFFFFFFFF;         // R1
           *(--psp) = 0xFFFFFFFF;         // R0


           // Update the TCB stack pointer
           tcb[i].sp = (void *)((uint32_t)psp & ~0x7);

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// function to restart a thread
void restartThread(_fn fn)
{
    __asm(" SVC #11");
}

// function to stop a thread
// remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    __asm(" SVC #9");
}

// function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm(" SVC #12");
}

// function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm(" SVC #0");
}

//function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
       __asm(" SVC #1"); // SVC #1 for yielding to the scheduler
}

//function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm(" SVC #2");
}

// this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm(" SVC #3");
}

//this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm(" SVC #4");
}

// this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm(" SVC #5");
}

// this function to add support for the system timer

// System Tick Interrupt Service Routine (ISR)
// This ISR is triggered at regular intervals by the SysTick timer.
// It handles decrementing delay ticks for delayed tasks and initiates preemption if enabled.

void systickIsr(void)
{
    uint8_t i;
    for (i = 0; i < MAX_TASKS; i++)
        {
            // Check if the task is delayed
            if (tcb[i].state == STATE_DELAYED)
            {
                // Decrement the tick count

                    tcb[i].ticks--;

                    // If the tick count reaches zero, set the task to ready
                    if (tcb[i].ticks == 0)
                    {
                        tcb[i].state = STATE_READY;
                    }

            }
        }
    if(preemption)      // Check if preemption is enabled
    {
        // Trigger the PendSV interrupt to perform a context switch
        NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;
    }


}



// in coop and preemptive, this function to add support for task switching
void pendSvIsr(void)
{
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;                  // Stop the timer
    tcb[taskCurrent].runtime += WTIMER0_TAV_R;         // Accumulate runtime for the current task
    WTIMER0_TAV_R = 0;
    __asm(" MRS     R0, PSP");                              // Load the PSP into a local register in the stack frame
    __asm(" STMDB   R0, {R4-R11, LR}");                     // Store registers R4-R11 and LR in the stack frame

    tcb[taskCurrent].sp = (void *)((uint32_t)getPSP() & ~0x7);                 // Store the PSP to the sp of the current task



    taskCurrent = rtosScheduler();
    applySramAccessMask(tcb[taskCurrent].srd);                    // Apply the SRD rules specific to the first thread
    setPSP((uint32_t)tcb[taskCurrent].sp);                 // Load the new PSP and execute

    WTIMER0_CTL_R |= TIMER_CTL_TAEN;                   // Start the timer for the new task


    __asm(" MRS     R1, PSP");                      // Load the PSP into a local register
    __asm(" SUBS    R1, #0x24");                    // Go down 9 registers and pop from there
    __asm(" LDMIA   R1!, {R4-R11, LR}");            // Load registers R4-R11 from the stack


}

// this function to add support for the service call
void svCallIsr(void)
{
    uint8_t i,j,k;
    uint32_t svcNumber;

    svcNumber = readSvcPriority();

    switch (svcNumber)
    {
        case 0: // Yield SVC
            NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;         // Trigger the PendSV interrupt to perform a context switch
            break;

        case 1: // SVC #1: Sleep request
            // Retrieve the tick count from R0

            // Set the task's ticks and mark it as delayed in TCB
            tcb[taskCurrent].ticks = moveToRegisterR0();
            tcb[taskCurrent].state = STATE_DELAYED;

            NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;        // Trigger the PendSV interrupt to perform a context switch

           break;
        case 2:    //lock svc
        {
            int8_t mutexId;
            mutexId = moveToRegisterR0();
            if (!mutexes[mutexId].lock)
            {
                // Lock the mutex and set the current task as the owner
                mutexes[mutexId].lock = true;
                mutexes[mutexId].lockedBy = taskCurrent;
            }
            else if(mutexes[mutexId].queueSize < MAX_MUTEX_QUEUE_SIZE)
                {
                    // Mutex is already locked, block the current task
                        uint8_t queueSize = mutexes[mutexId].queueSize;

                        // Add the current task to the mutex's waiting queue
                        mutexes[mutexId].processQueue[queueSize] = taskCurrent;
                        mutexes[mutexId].queueSize++;

                        // Block the current task on this mutex
                        tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
                        tcb[taskCurrent].mutex = mutexId;



                }
                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;      // Trigger the PendSV interrupt to perform a context switch
                    break;
        }




        case 3:    //unlock SVC
        {
            int8_t mutexId;
            mutexId = moveToRegisterR0();
            if (mutexes[mutexId].lockedBy == taskCurrent)
            {
                // Release the lock
                mutexes[mutexId].lock = false;
                mutexes[mutexId].lockedBy = 0;       // No owner

                // If there are tasks waiting, unblock the first one
                if (mutexes[mutexId].queueSize > 0)
                {
                    uint8_t nextTask = mutexes[mutexId].processQueue[0];
                    uint8_t i;
                    for (i = 1; i < mutexes[mutexId].queueSize; i++)
                    {
                        mutexes[mutexId].processQueue[i - 1] = mutexes[mutexId].processQueue[i];
                    }
                    mutexes[mutexId].queueSize--;

                    tcb[nextTask].state = STATE_READY;
                    mutexes[mutexId].lock = true;
                    mutexes[mutexId].lockedBy = nextTask;
                }
                break;
            }
            NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;   // Trigger the PendSV interrupt to perform a context switch
            break;
        }
        case 4:   //wait SVC
        {
            int8_t semaphoreId;
            semaphoreId = moveToRegisterR0();
            if (semaphores[semaphoreId].count > 0)
            {
                // Decrement the semaphore count
                semaphores[semaphoreId].count--;
            }
            else
            {
                // Block the current task
                semaphores[semaphoreId].processQueue[semaphores[semaphoreId].queueSize++] = taskCurrent;
                tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
                tcb[taskCurrent].semaphore = semaphoreId; // Store semaphore causing block

                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;       // Trigger the PendSV interrupt to perform a context switch
            }
            break;
        }
        case 5:   //Post  SVC
        {
            int8_t semaphoreId;
            semaphoreId = moveToRegisterR0();
            if (semaphores[semaphoreId].queueSize > 0)
            {
                // Unblock the first task in the queue
                uint8_t nextTask = semaphores[semaphoreId].processQueue[0];

                for (i = 1; i < semaphores[semaphoreId].queueSize; i++)
                {
                    semaphores[semaphoreId].processQueue[i - 1] = semaphores[semaphoreId].processQueue[i];
                }
                semaphores[semaphoreId].queueSize--;

                // Set the task to READY state
                tcb[nextTask].state = STATE_READY;
                tcb[nextTask].semaphore = 0xFF; // Clear semaphore blocking info
            }
            else
            {
                // Increment the semaphore count if no task is waiting
                semaphores[semaphoreId].count++;
            }
            NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;       // Trigger the PendSV interrupt to perform a context switch
            break;
        }
        case 6:      //preempt SVC
        {
            volatile bool p = moveToRegisterR0();
            preemption = p;
            if(preemption)
            {
                putsUart0("preeempt on");
            }
            if (!preemption)
            {
                putsUart0("preeempt off");
            }
            NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;       // Trigger the PendSV interrupt to perform a context switch
            break;
        }
        case 7:     // priorityScheduler SVC
        {
            volatile bool schedulerMode = (bool)moveToRegisterR0();

            // Update the global variable
            priorityScheduler = schedulerMode;
            if(priorityScheduler)
            {
                putsUart0("priorityScheduler on");
            }
            if(!priorityScheduler)
            {
                putsUart0("priorityScheduler off");
            }
            NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;       // Trigger the PendSV interrupt to perform a context switch
            break;

        }
        case 8:    //pkill SVC
        {
            char* procName = (char*)moveToRegisterR0();
            uint8_t i;

            for (i = 0; i < MAX_TASKS; i++) // Iterate over all tasks
            {
                if (compare_string(tcb[i].name, procName)) // Match the function name in the TCB
                {
                    uint8_t mutexId = tcb[i].mutex;

                    // If the task owns a mutex, release it
                    if (mutexes[mutexId].lockedBy == i)
                    {
                        mutexes[mutexId].lock = false;
                        mutexes[mutexId].lockedBy = 0; // No owner

                        // Handle the mutex queue
                        if (mutexes[mutexId].queueSize > 0)
                        {
                            uint8_t nextTask = mutexes[mutexId].processQueue[0];

                            // Shift queue to remove the first element
                            for (j = 0; j < mutexes[mutexId].queueSize - 1; j++)
                            {
                                mutexes[mutexId].processQueue[j] = mutexes[mutexId].processQueue[j + 1];
                            }

                            mutexes[mutexId].queueSize--; // Decrement queue size

                            // Assign mutex to the next task in the queue
                            tcb[nextTask].state = STATE_READY;
                            mutexes[mutexId].lock = true;
                            mutexes[mutexId].lockedBy = nextTask;
                        }
                    }
                    if (tcb[i].state == STATE_BLOCKED_MUTEX)
                    {
                        uint8_t mutexId = tcb[i].mutex;

                        // Iterate over the mutex queue to find the task
                        for (j = 0; j < mutexes[mutexId].queueSize; j++)
                        {
                            if (mutexes[mutexId].processQueue[j] == i)  // Match task ID
                            {
                                // Shift the queue to remove the task
                                for (k = j; k < mutexes[mutexId].queueSize - 1; k++)
                                {
                                    mutexes[mutexId].processQueue[k] = mutexes[mutexId].processQueue[k + 1];
                                }
                                mutexes[mutexId].queueSize--; // Decrement queue size
                                break;
                            }
                        }
                    }

                    // If the task is blocked by a semaphore, remove it from the queue
                    if (tcb[i].state == STATE_BLOCKED_SEMAPHORE)
                    {
                        uint8_t semaphoreId = tcb[i].semaphore;

                        for (j = 0; j < semaphores[semaphoreId].queueSize; j++)
                        {
                            if (semaphores[semaphoreId].processQueue[j] == i)
                            {
                                for (k = j; k < semaphores[semaphoreId].queueSize - 1; k++)
                                {
                                    semaphores[semaphoreId].processQueue[k] = semaphores[semaphoreId].processQueue[k + 1];
                                }
                                semaphores[semaphoreId].queueSize--;
                                break;
                            }
                        }
                    }

                    // Mark the thread as stopped and clear its TCB values
                    tcb[i].state = STATE_STOPPED;
                    tcb[i].mutex = 0;
                    tcb[i].semaphore = 0;
                    tcb[i].ticks = 0;

                    break;
                }
            }

            // Trigger PendSV to perform context switching
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
        case 9:    //kill
        {
            uint32_t pid = (uint32_t)moveToRegisterR0();

            for (i = 0; i < MAX_TASKS; i++)
            {
                if ((uint32_t)tcb[i].pid == pid)
                {
                    uint8_t mutexId = tcb[i].mutex;

                    // If the task owns a mutex, release it
                    if (mutexes[mutexId].lockedBy == i)
                    {
                        mutexes[mutexId].lock = false;
                        mutexes[mutexId].lockedBy = 0; // No owner

                        // Handle the mutex queue
                        if (mutexes[mutexId].queueSize > 0)
                        {
                            uint8_t nextTask = mutexes[mutexId].processQueue[0];

                            // Shift queue to remove the first element
                            for (j = 0; j < mutexes[mutexId].queueSize - 1; j++)
                            {
                                mutexes[mutexId].processQueue[j] = mutexes[mutexId].processQueue[j + 1];
                            }

                            mutexes[mutexId].queueSize--; // Decrement queue size

                            // Assign mutex to the next task in the queue
                            tcb[nextTask].state = STATE_READY;
                            mutexes[mutexId].lock = true;
                            mutexes[mutexId].lockedBy = nextTask;
                        }
                    }
                    if (tcb[i].state == STATE_BLOCKED_MUTEX)
                    {
                        uint8_t mutexId = tcb[i].mutex;

                        // Iterate over the mutex queue to find the task
                        for (j = 0; j < mutexes[mutexId].queueSize; j++)
                        {
                            if (mutexes[mutexId].processQueue[j] == i)  // Match task ID
                            {
                                // Shift the queue to remove the task
                                for (k = j; k < mutexes[mutexId].queueSize - 1; k++)
                                {
                                    mutexes[mutexId].processQueue[k] = mutexes[mutexId].processQueue[k + 1];
                                }
                                mutexes[mutexId].queueSize--; // Decrement queue size
                                break;
                            }
                        }
                    }

                    // If the task is blocked by a semaphore, remove it from the queue
                    if (tcb[i].state == STATE_BLOCKED_SEMAPHORE)
                    {
                        uint8_t semaphoreId = tcb[i].semaphore;

                        for (j = 0; j < semaphores[semaphoreId].queueSize; j++)
                        {
                            if (semaphores[semaphoreId].processQueue[j] == i)
                            {
                                for (k = j; k < semaphores[semaphoreId].queueSize - 1; k++)
                                {
                                    semaphores[semaphoreId].processQueue[k] = semaphores[semaphoreId].processQueue[k + 1];
                                }
                                semaphores[semaphoreId].queueSize--;
                                break;
                            }
                        }
                    }

                    // Mark the thread as stopped and clear its TCB values
                    tcb[i].state = STATE_STOPPED;
                    tcb[i].mutex = 0;
                    tcb[i].semaphore = 0;
                    tcb[i].ticks = 0;

                    break;
                }
            }

            // Trigger PendSV to perform context switching
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
        case 10:   //pidof SVC
        {
            char *proc_name = (char *)moveToRegisterR0();
            uint32_t pid;
            char itoa_str[20];
            // Search for the task by name
            for (i = 0; i < MAX_TASKS; i++)
            {
                if (compare_string(tcb[i].name, proc_name))
                {
                    pid = (uint32_t)tcb[i].pid; // Get the PID of the matching task
                    itoa(pid, itoa_str);
                    putsUart0(itoa_str);
                    break;
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
        case 11:      //Restart SVC
        {
            uint32_t pid = (uint32_t)moveToRegisterR0();
            for(i = 0; i < MAX_TASKS; i++)
            {
                if((uint32_t)tcb[i].pid == pid)
                {
                    tcb[i].state = STATE_READY;                                             // Update the state to ready
                    break;

                }

            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
        case 12:   // set priority
        {
            uint32_t cppid = (uint32_t)moveToRegisterR0();
            uint32_t *psp = (uint32_t *)getPSP();
            uint32_t priority = *(psp + 1);
            for(i = 0; i < MAX_TASKS; i++)
            {
                if((uint32_t)tcb[i].pid == cppid)
                {
                    tcb[i].currentPriority = priority;
                    break;

                }

            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;

        }
        case 13: //Reboot SVC
        {
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
        }
        case 14:
        {
            IPCSInfo* userIPCSInfo = (IPCSInfo*)moveToRegisterR0(); // Get the address from R0

            // Copy mutex data
            for (i = 0; i < MAX_MUTEXES; i++)
            {
                userIPCSInfo->mutexes[i].lock = mutexes[i].lock;
                userIPCSInfo->mutexes[i].queueSize = mutexes[i].queueSize;
                userIPCSInfo->mutexes[i].lockedBy = mutexes[i].lockedBy;
                for (j = 0; j < MAX_MUTEX_QUEUE_SIZE; j++)
                {
                    userIPCSInfo->mutexes[i].processQueue[j] = mutexes[i].processQueue[j];
                }
            }

            // Copy semaphore data
            for (i = 0; i < MAX_SEMAPHORES; i++)
            {
                userIPCSInfo->semaphores[i].count = semaphores[i].count;
                userIPCSInfo->semaphores[i].queueSize = semaphores[i].queueSize;
                for (j = 0; j < MAX_SEMAPHORE_QUEUE_SIZE; j++)
                {
                    userIPCSInfo->semaphores[i].processQueue[j] = semaphores[i].processQueue[j];
                }
            }


            break;
        }
        case 15:      //Restart SVC
        {
            char *proc_name = (char *)moveToRegisterR0();
            for(i = 0; i < taskCount; i++)
            {
                if(compare_string(tcb[i].name, proc_name))
                {
                    tcb[i].state = STATE_READY;                                             // Update the state to ready
                    break;

                }

            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
            break;
        }
        case 16: // mallocFromHeap
        {
            uint32_t size_in_bytes = moveToRegisterR0(); // Retrieve size from R0
            uint32_t *psp = (uint32_t*) getPSP();
            void** ptr1 = (void **)*(psp + 1);

            void *allocated = mallocFromHeap(size_in_bytes); // Perform allocation
            *ptr1 = allocated;

            uint64_t srdBitMask = setSramAccessWindow((uint32_t *)ptr1, size_in_bytes);
            applySramAccessMask(srdBitMask);
            break;

        }
        case 17: // freeToHeap
        {
            void* pMemory = (void*)moveToRegisterR0(); // Retrieve pointer from R0
            freeToHeap(pMemory); // Perform memory deallocation
            //moveToRegisterR0WithValue(1); // Indicate success (e.g., 1 = success)
            break;
        }
        case 18: // ps command
        {
            PSInfo *psInfo = (PSInfo *)moveToRegisterR0();  // Get pointer to PSInfo structure

            // Populate process information
            psInfo->taskCount = taskCount;
            uint32_t totalRuntime = 0;

            // Calculate total runtime
            for (i = 0; i < taskCount; i++)
                totalRuntime += tcb[i].runtime;

            // Fill process details
            for (i = 0; i < taskCount; i++)
            {
                psInfo->tasks[i].pid = (uint32_t)tcb[i].pid;
                manualStringCopy(psInfo->tasks[i].name, tcb[i].name, sizeof(psInfo->tasks[i].name));
                psInfo->tasks[i].state = tcb[i].state;

                // Calculate CPU percentage
                if (totalRuntime > 0)
                    psInfo->tasks[i].cpuPercent = (tcb[i].runtime * 100) / totalRuntime;
                else
                    psInfo->tasks[i].cpuPercent = 0;

                // Determine blocking resource
                if (tcb[i].state == STATE_BLOCKED_MUTEX)
                {
                    psInfo->tasks[i].blockingResourceType = 1;
                    psInfo->tasks[i].blockingResourceId = tcb[i].mutex;
                }
                else if (tcb[i].state == STATE_BLOCKED_SEMAPHORE)
                {
                    psInfo->tasks[i].blockingResourceType = 2;
                    psInfo->tasks[i].blockingResourceId = tcb[i].semaphore;
                }
                else
                {
                    psInfo->tasks[i].blockingResourceType = 0;
                    psInfo->tasks[i].blockingResourceId = 0xFF;
                }
            }

            break;
        }


    }
}


