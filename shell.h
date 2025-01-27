// Shell functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef SHELL_H_
#define SHELL_H_

#define SHELL_MAX_MUTEX_QUEUE_SIZE 2
#define SHELL_MAX_SEMAPHORE_QUEUE_SIZE 2
#define SHELL_MAX_MUTEXES 1
#define SHELL_MAX_SEMAPHORES 3
#define SHELL_MAX_TASKS 12

// task states
#define SHELL_STATE_INVALID           0 // no task
#define SHELL_STATE_STOPPED           1 // stopped, all memory freed
#define SHELL_STATE_READY             2 // has run, can resume at any time
#define SHELL_STATE_DELAYED           3 // has run, but now awaiting timer
#define SHELL_STATE_BLOCKED_MUTEX     4 // has run, but now blocked by semaphore
#define SHELL_STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

typedef struct
{
    uint32_t pid;                 // Process ID
    char name[16];                // Process name
    uint8_t state;                // Process state
    uint32_t cpuPercent;          // CPU usage percentage
    uint8_t blockingResourceType; // 0=none, 1=mutex, 2=semaphore
    uint8_t blockingResourceId;   // Index of the blocking mutex/semaphore
} ProcessStatus;

typedef struct
{
    ProcessStatus tasks[SHELL_MAX_TASKS];
    uint8_t taskCount;
} PSInfo;

typedef struct
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[SHELL_MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} MutexInfo;

typedef struct
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[SHELL_MAX_SEMAPHORE_QUEUE_SIZE];
} SemaphoreInfo;

typedef struct
{
    MutexInfo mutexes[SHELL_MAX_MUTEXES];
    SemaphoreInfo semaphores[SHELL_MAX_SEMAPHORES];
} IPCSInfo;






//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shell(void);

#endif
