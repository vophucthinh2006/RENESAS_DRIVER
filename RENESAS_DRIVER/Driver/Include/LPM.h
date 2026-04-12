#ifndef LPM_H
#define LPM_H
#include <stdint.h>

#define SYSC       0x4001E000UL

#define MSTPCRB    *(volatile uint32_t*)(uintptr_t)(SYSC + 0x700)
#define MSTPCRC    *(volatile uint32_t*)(uintptr_t)(SYSC + 0x704)
#define MSTPCRD    *(volatile uint32_t*)(uintptr_t)(SYSC + 0x708)

typedef enum{
    SCI0,
    SCI1,
    SCI2,
    SCI3,
    SCI4,
    SCI5,
    SCI6,
    SCI7,
    SCI8,
    SCI9
}SCI_t;

void LPM_Unlock(SCI_t peripheral);

#endif
