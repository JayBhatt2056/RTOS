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

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "faults.h"
#include "Registers.h"
#include "uart0.h"
#include "string.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    uint32_t *psp;

    psp = (uint32_t *)getPSP();
    char* p1 = hexToString(psp[0]);
    putsUart0("\n");
    putsUart0("R0 >");
    putsUart0(p1);
    char* p2 = hexToString(psp[1]);
    putsUart0("\n");
    putsUart0("R1 >");
    putsUart0(p2);
    char* p3 = hexToString(psp[2]);
    putsUart0("\n");
    putsUart0("R2 >");
    putsUart0(p3);
    char* p4 = hexToString(psp[3]);
    putsUart0("\n");
    putsUart0("R3 >");
    putsUart0(p4);
    char* p5 = hexToString(psp[4]);
    putsUart0("\n");
    putsUart0("R12 >");
    putsUart0(p5);
    char* p6 = hexToString(psp[5]);
    putsUart0("\n");
    putsUart0("LR >");
    putsUart0(p6);
    char* p7 = hexToString(psp[6]);
    putsUart0("\n");
    putsUart0("PC >");
    putsUart0(p7);
    char* p8 = hexToString(psp[7]);
    putsUart0("\n");
    putsUart0("XSPR >");
    putsUart0(p8);
    putsUart0("\n");

    char* p9 = hexToString((uint32_t)NVIC_MM_ADDR_R);
    putsUart0("\n");
    putsUart0("Offending Data Address >");
    putsUart0(p9);

    char* p11 = hexToString((uint32_t)NVIC_FAULT_STAT_R);
    putsUart0("\n");
    putsUart0("Fault flags >");
    putsUart0(p11);

    putsUart0("\n");
    char* p = hexToString(getMSP());
    putsUart0("psp >");
    putsUart0(p);
    char* m = hexToString(getPSP());
    putsUart0("\n");
    putsUart0("msp >");
    putsUart0(m);
    NVIC_SYS_HND_CTRL_R &= ~(NVIC_SYS_HND_CTRL_MEMP);
    putsUart0("MPU fault in process");

    NVIC_INT_CTRL_R     |= NVIC_INT_CTRL_PEND_SV;       ////ENABLE pendsv FAULTS
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    uint32_t *psp;
    psp = (uint32_t *)getPSP();
    putsUart0("hard fault in process");


    char* p1 = hexToString(psp[0]);
    putsUart0("\n");
    putsUart0("R0 >");
    putsUart0(p1);
    char* p2 = hexToString(psp[1]);
    putsUart0("\n");
    putsUart0("R1 >");
    putsUart0(p2);
    char* p3 = hexToString(psp[2]);
    putsUart0("\n");
    putsUart0("R2 >");
    putsUart0(p3);
    char* p4 = hexToString(psp[3]);
    putsUart0("\n");
    putsUart0("R3 >");
    putsUart0(p4);
    char* p5 = hexToString(psp[4]);
    putsUart0("\n");
    putsUart0("R12 >");
    putsUart0(p5);
    char* p6 = hexToString(psp[5]);
    putsUart0("\n");
    putsUart0("LR >");
    putsUart0(p6);
    char* p7 = hexToString(psp[6]);
    putsUart0("\n");
    putsUart0("PC >");
    putsUart0(p7);
    char* p8 = hexToString(psp[7]);
    putsUart0("\n");
    putsUart0("XSPR >");
    putsUart0(p8);
    putsUart0("\n");

    char* p9 = hexToString((uint32_t)NVIC_FAULT_STAT_R);
    putsUart0("\n");
    putsUart0("Fault flags >");
    putsUart0(p9);





    putsUart0("\n");
    char* p = hexToString(getMSP());
    putsUart0("psp >");
    putsUart0(p);
    char* m = hexToString(getPSP());
    putsUart0("\n");
    putsUart0("msp >");
    putsUart0(m);
    char* p10 = hexToString(psp[6]);
    putsUart0("\n");
    putsUart0("Offending Instruction >");
    putsUart0(p10);


    NVIC_HFAULT_STAT_R  |= NVIC_HFAULT_STAT_DBG |NVIC_HFAULT_STAT_FORCED |NVIC_HFAULT_STAT_VECT;

    while(1);
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    NVIC_FAULT_STAT_R   &= ~NVIC_FAULT_STAT_IBUS;  //clear bus fault
    putsUart0("Bus fault in process PDI");
    while(1);
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    NVIC_CFG_CTRL_R     &= ~NVIC_CFG_CTRL_DIV0;
    putsUart0("usage fault in process PID");
    while(1);
}

