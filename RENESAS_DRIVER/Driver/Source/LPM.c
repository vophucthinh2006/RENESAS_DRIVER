#include "LPM.h"

/*
 * LPM_Unlock - Enable the module stop clock for an SCI peripheral.
 *
 * MSTPCRB register (RA6M5 HW manual §9.3.3):
 *   Bit = 1: module clock STOPPED (reset default)
 *   Bit = 0: module clock RUNNING  <-- we must CLEAR the bit to enable
 *
 * SCI channel to MSTPCRB bit mapping:
 *   SCI0 -> bit31, SCI1 -> bit30, ..., SCI9 -> bit22
 */
void LPM_Unlock(SCI_t peripheral)
{
    switch (peripheral)
    {
        case SCI0: MSTPCRB &= ~(1UL << 31); break;
        case SCI1: MSTPCRB &= ~(1UL << 30); break;
        case SCI2: MSTPCRB &= ~(1UL << 29); break;
        case SCI3: MSTPCRB &= ~(1UL << 28); break;
        case SCI4: MSTPCRB &= ~(1UL << 27); break;
        case SCI5: MSTPCRB &= ~(1UL << 26); break;
        case SCI6: MSTPCRB &= ~(1UL << 25); break;
        case SCI7: MSTPCRB &= ~(1UL << 24); break;
        case SCI8: MSTPCRB &= ~(1UL << 23); break;
        case SCI9: MSTPCRB &= ~(1UL << 22); break;
        default: break;
    }
}
