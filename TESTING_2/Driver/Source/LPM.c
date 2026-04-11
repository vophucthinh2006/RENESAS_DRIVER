#include "LPM.h"

void LPM_Unlock(SCI_t peripheral){
    switch (peripheral){
        case SCI0: MSTPCRB |= (1 << 31); break;
        case SCI1: MSTPCRB |= (1 << 30); break;
        case SCI2: MSTPCRB |= (1 << 29); break;
        case SCI3: MSTPCRB |= (1 << 28); break;
        case SCI4: MSTPCRB |= (1 << 27); break;
        case SCI5: MSTPCRB |= (1 << 26); break;
        case SCI6: MSTPCRB |= (1 << 25); break;
        case SCI7: MSTPCRB |= (1 << 24); break;
        case SCI8: MSTPCRB |= (1 << 23); break;
        case SCI9: MSTPCRB |= (1 << 22); break;
        default: break;
    }
}
