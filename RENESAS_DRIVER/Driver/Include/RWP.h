#ifndef RWP_H
#define RWP_H
#include <stdint.h>
#include "drv_common.h"

#define PRCR        *(volatile uint16_t*)(uintptr_t)(SYSC + 0x3FE)

void RWP_Unlock_Clock_MSTP();
void RWP_Lock_Clock_MSTP();

#endif
