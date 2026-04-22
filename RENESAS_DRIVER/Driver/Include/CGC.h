#ifndef CGC_H
#define CGC_H
#include <stdint.h>
#include "drv_common.h"
/* SYSC base address is defined in drv_common.h */

#define HOCOCR      *(volatile uint8_t*)(uintptr_t)(SYSC + 0x036)
#define MOCOCR      *(volatile uint8_t*)(uintptr_t)(SYSC + 0x038)

#define PLLCCR      *(volatile uint16_t*)(uintptr_t)(SYSC + 0x028)
#define PLLCR       *(volatile uint8_t*)(uintptr_t)(SYSC + 0x02A)

#define SCKSCR      *(volatile uint8_t*)(uintptr_t)(SYSC + 0x026)

#define SCKDIVCR    *(volatile uint32_t*)(uintptr_t)(SYSC + 0x020)

#define OSCSF       *(volatile uint8_t*)(uintptr_t)(SYSC + 0x03C)
#define PLLSR       *(volatile uint8_t*)(uintptr_t)(SYSC + 0x02C)

#endif
