#ifndef RWP_H
#define RWP_H
#include <stdint.h>

#define SYSC        0x4001E000UL

#define PRCR        *(volatile uint16_t*)(uintptr_t)(SYSC + 0x3FE)

void RWP_Unlock_Clock_MSTP();
void RWP_Lock_Clock_MSTP();

#endif
