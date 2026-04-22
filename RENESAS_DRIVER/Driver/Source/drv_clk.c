#include "drv_clk.h"
#include "drv_rwp.h"

/* -----------------------------------------------------------------------
 * CLK_Init — explicit MOCO 8 MHz system clock configuration.
 *
 * RA6M5 resets with MOCO selected (SCKSCR=0x01) and all clock dividers
 * at /1 (SCKDIVCR=0x00000000). This function makes that state explicit
 * and is safe to call as the very first action in Reset_Handler.
 *
 * Clock tree after CLK_Init (Phase 6 — MOCO 8 MHz baseline):
 *   Source : MOCO (8 MHz)
 *   ICLK   : 8 MHz  (ICK  = /1)
 *   PCLKB  : 8 MHz  (PCKB = /1)  ← matches PCLKB in drv_uart.h
 *   PCLKA  : 8 MHz  (PCKA = /1)
 *   BCLK   : 8 MHz  (BCK  = /1)
 *   FCLK   : 8 MHz  (FCK  = /1)
 *
 * To switch to HOCO 64 MHz later:
 *   1. MOCOCR.MCSTP = 1  (stop MOCO — optional)
 *   2. HOCOCR.HCSTP = 0  (start HOCO)
 *   3. Wait OSCSF.HOCOSF = 1  (stable)
 *   4. Set SCKDIVCR as required
 *   5. SCKSCR = SCKSCR_HOCO
 * ----------------------------------------------------------------------- */
void CLK_Init(void)
{
    RWP_Unlock_Clock_MSTP();

    /* All system clock dividers = /1 (explicit, matches reset default) */
    SCKDIVCR = ((uint32_t)SCKDIV_1 << SCKDIVCR_ICKPOS)   /* ICLK  = /1 */
             | ((uint32_t)SCKDIV_1 << SCKDIVCR_PCKBPOS)   /* PCLKB = /1 */
             | ((uint32_t)SCKDIV_1 << SCKDIVCR_PCKAPOS)   /* PCLKA = /1 */
             | ((uint32_t)SCKDIV_1 << SCKDIVCR_PCKDPOS)   /* PCLKD = /1 */
             | ((uint32_t)SCKDIV_1 << SCKDIVCR_BCKPOS)    /* BCLK  = /1 */
             | ((uint32_t)SCKDIV_1 << SCKDIVCR_FCKPOS);   /* FCLK  = /1 */

    /* Select MOCO (8 MHz) as system clock source */
    SCKSCR = SCKSCR_MOCO;

    RWP_Lock_Clock_MSTP();
}

/* -----------------------------------------------------------------------
 * CLK_ModuleStart_SCI — release module stop for one SCI channel.
 *
 * Replaces LPM_Unlock(). Wraps MSTPCRB write with PRCR protection.
 * MSTPCRB bit mapping: SCI0=bit31, SCI1=bit30, ..., SCI9=bit22
 * ----------------------------------------------------------------------- */
void CLK_ModuleStart_SCI(SCI_t peripheral)
{
    RWP_Unlock_Clock_MSTP();

    switch (peripheral)
    {
        case SCI0: MSTPCRB &= ~(1UL << 31U); break;
        case SCI1: MSTPCRB &= ~(1UL << 30U); break;
        case SCI2: MSTPCRB &= ~(1UL << 29U); break;
        case SCI3: MSTPCRB &= ~(1UL << 28U); break;
        case SCI4: MSTPCRB &= ~(1UL << 27U); break;
        case SCI5: MSTPCRB &= ~(1UL << 26U); break;
        case SCI6: MSTPCRB &= ~(1UL << 25U); break;
        case SCI7: MSTPCRB &= ~(1UL << 24U); break;
        case SCI8: MSTPCRB &= ~(1UL << 23U); break;
        case SCI9: MSTPCRB &= ~(1UL << 22U); break;
        default:   break;
    }

    RWP_Lock_Clock_MSTP();
}
