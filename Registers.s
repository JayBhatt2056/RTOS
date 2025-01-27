.thumb
.const




    .def setASP
	.def getPSP
    .def getMSP
	.def setPSP
	.def enablePrivilegedMode
	.def disablePrivilegedMode
	.def startRtosAssembly
	.def moveToRegisterR0
	.def readSvcPriority
	.def moveToRegisterR0WithValue




setPSP:
    MSR PSP, R0         ; Move the value in R0 (the passed argument) into the PSP register
    ISB                 ; Instruction Synchronization Barrier
    BX LR               ; Return to the calling function
getPSP:
    MRS R0, PSP         ; Read the PSP register
    ISB                 ; Wait for sync
    BX LR               ; Return to the calling function

getMSP:
    MRS R0, MSP         ; Read the MSP register
    ISB                 ; Wait for sync
    BX LR               ; Return to the calling function

setASP:
    MRS R1, CONTROL     ; Read the current CONTROL register value
    ORR R1, R1, #0x02   ; Set the ASP bit
    MSR CONTROL, R1     ; Load the new CONTROL register value
    ISB                 ; Instruction sync
    BX LR               ; Return


disablePrivilegedMode:
    MRS R1, CONTROL     ; Read the current CONTROL register value
    ORR R1, R1, #0x01   ; Set the TMPL bit
    MSR CONTROL, R1     ; Write the updated value back to CONTROL register
    ISB                 ; Instruction Synchronization Barrier (ensure proper execution order)
    BX LR

enablePrivilegedMode:
    MRS R0, CONTROL     ; Read the current CONTROL register value
    BIC R0, R0, #0x03   ; Clear the ASP and TMPL bits
    MSR CONTROL, R0     ; Write the updated value back to CONTROL register
    ISB                 ; Instruction Synchronization Barrier
    BX LR             ; Return from function


startRtosAssembly:
    MSR PSP, R0         ; Load the address into PSP register
    ISB                 ; Instruction sync
    MRS R1, CONTROL     ; Read the current CONTROL register value
    ORR R1, R1, #0x02   ; Set ASP bit in CONTROL register
    MSR CONTROL, R1     ; Load the new CONTROL register value
    ISB                 ; Instruction sycn
    BX  LR              ; Return


moveToRegisterR0:            ; Define the new function
   	MRS R0, PSP         ; Load PSP into R0 to determine which function made the SV Call
    LDR R0, [R0]        ; Derefernce the value from PSP pointer
    BX  LR

moveToRegisterR0WithValue:
   	STR R0, [R1]             ; Store the value in R0 to the memory location pointed by R1
	BX LR                    ; Return to the caller

readSvcPriority:
    MRS R0, PSP         ; Load PSP into R0 to determine which function made the SV Call
    LDR R0, [R0, #24]   ; Load return address of that function
    LDRB R0, [R0, #-2]  ; Get the value of the argument from the location before the return address pointing to
    BX  LR              ; Return

