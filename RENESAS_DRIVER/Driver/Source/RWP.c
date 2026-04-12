#include "RWP.h"

void RWP_Unlock_Clock_MSTP(){
    PRCR = (uint16_t)(0xA5 << 8| 0b00000011);
}
void RWP_Lock_Clock_MSTP(){
    PRCR = (uint16_t)(0xA5 << 8);
}
