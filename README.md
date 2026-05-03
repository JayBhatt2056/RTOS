# RTOS for TM4C123GH6PM

This project is a custom real-time operating system built for the TI TM4C123GH6PM Cortex-M4F microcontroller on the EK-TM4C123GXL LaunchPad. It focuses on core RTOS mechanisms such as task scheduling, synchronization, memory isolation, fault handling, and UART-based runtime control.

The codebase is structured as a Code Composer Studio project and is aimed at embedded systems learning, experimentation, and demonstration on real hardware.

## Features

- Preemptive or cooperative task switching
- Priority-based scheduling with round-robin fallback
- Thread creation with per-task stacks and priorities
- Mutex and semaphore synchronization primitives
- Sleep, yield, kill, restart, and priority-change task controls
- Heap allocation support backed by SRAM region management
- MPU-oriented SRAM access windows for task isolation
- Fault handlers for MPU, hard fault, bus fault, and usage fault events
- UART shell for inspecting and controlling the system at runtime
- Per-task runtime accounting used by the `ps` command

## Hardware Target

- Board: EK-TM4C123GXL LaunchPad
- MCU: TM4C123GH6PM
- Core: ARM Cortex-M4F
- System clock: 40 MHz
- UART console: `UART0` at `115200 8-N-1`
- I/O used by the demo: 6 pushbuttons and 5 LEDs

The shell is exposed over `UART0`, and the demo tasks use the board plus attached external LEDs/buttons defined in [tasks.c](tasks.c).

## What The RTOS Does

At startup, the system initializes the clock, GPIO, timer, UART, fault handlers, and MPU configuration before creating a set of demo threads. The kernel uses `SysTick` for the 1 ms scheduler tick and `PendSV` for context switching, while SVC handlers provide the system call interface for thread and synchronization operations.

The project demonstrates:

- Task scheduling with both priority and round-robin modes
- Runtime toggling of preemption
- Blocking and wake-up behavior through mutexes and semaphores
- Controlled task delays with `sleep`
- Dynamic stack and heap allocation
- Memory fault visibility through UART diagnostics

## Demo Tasks

The application starts several built-in tasks from [rtos.c](rtos.c):

- `Idle`: always-available background task required by the scheduler
- `LengthyFn`: long-running mutex-protected workload with heap allocation
- `Flash4Hz`: periodic LED flasher
- `OneShot`: waits on a semaphore and flashes an LED once per event
- `ReadKeys`: reads pushbutton input and triggers task actions
- `Debounce`: debounces button presses
- `Important`: protected critical-section example
- `Uncoop`: intentionally uncooperative workload for scheduler testing
- `Errant`: intentionally invalid memory access for fault testing
- `Shell`: UART command interface

## UART Shell Commands

The shell implementation lives in [shell.c](shell.c). Supported commands include:

| Command | Description |
| --- | --- |
| `ps` | Show task PID, name, state, CPU usage, and blocking resource |
| `ipcs` | Display mutex and semaphore state, ownership, and wait queues |
| `kill <pid>` | Stop a task by PID |
| `pkill <name>` | Stop a task by task name |
| `Pidof <name>` | Print the PID for a task name |
| `preempt on` | Enable preemptive scheduling |
| `preempt off` | Disable preemption and rely on cooperative yielding |
| `sched prio` | Use priority scheduling |
| `sched rr` | Use round-robin scheduling |
| `reboot` | Reboot the microcontroller |
| `<task name>` | Mark a matching task as ready again |

Command matching is case-sensitive as currently implemented, so use the command names exactly as shown above.

## Project Layout

- [rtos.c](rtos.c): system startup, hardware initialization, and thread creation
- [kernel.c](kernel.c) / [kernel.h](kernel.h): scheduler, context switching, SVC handlers, mutexes, semaphores, and task control
- [mm.c](mm.c) / [mm.h](mm.h): heap allocator and SRAM/MPU region handling
- [faults.c](faults.c) / [faults.h](faults.h): exception and fault handlers
- [tasks.c](tasks.c) / [tasks.h](tasks.h): application demo tasks and I/O behavior
- [shell.c](shell.c) / [shell.h](shell.h): UART shell and status structures
- [Registers.s](Registers.s) / [Registers.h](Registers.h): low-level register helpers and assembly support
- [tm4c123gh6pm_startup_ccs.c](tm4c123gh6pm_startup_ccs.c): startup vectors
- [tm4c123gh6pm.cmd](tm4c123gh6pm.cmd): linker command file

## Build And Run

1. Open Code Composer Studio.
2. Import the repository root as an existing CCS project.
3. Connect the EK-TM4C123GXL LaunchPad.
4. Build the project and flash it to the board.
5. Open a serial terminal on the LaunchPad's virtual COM port at `115200 8-N-1`.
6. Use the UART shell to inspect tasks, change scheduler behavior, and test fault handling.

Because the repository already includes `.project`, `.cproject`, and `.ccsproject`, it is set up to be imported directly into Code Composer Studio.

## Notes

- This project is tightly coupled to the TM4C123GH6PM memory map and peripherals.
- Pin assignments for LEDs and pushbuttons are defined in [tasks.c](tasks.c).
- The memory-management and MPU code is designed around fixed SRAM regions and subregion masks rather than a general-purpose allocator.
