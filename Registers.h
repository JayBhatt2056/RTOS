#ifndef REGISTERS_H
#define REGISTERS_H

#include <inttypes.h>




extern void setPSP(uint32_t P);
extern void setASP(void);
extern uint32_t getPSP(void);
extern uint32_t getMSP(void);
extern uint32_t enablePrivilegedMode(void);
extern uint32_t disablePrivilegedMode(void);
extern void startRtosAssembly(uint32_t stack_ptr);
extern uint32_t moveToRegisterR0(void);
extern void moveToRegisterR0WithValue(uint32_t value);
extern uint32_t readSvcPriority(void);
extern void* SVCmallocFromHeap(uint32_t size_in_bytes);
#endif
