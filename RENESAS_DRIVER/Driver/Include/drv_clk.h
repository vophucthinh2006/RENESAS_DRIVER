#ifndef DRV_CLK_H
#define DRV_CLK_H
#include <stdint.h>
#include "drv_common.h"

/*
 * drv_clk.h — Clock and Module Stop control for RA6M5.
 *
 * Replaces: CGC.h (clock registers) + LPM.h (MSTPCR registers).
 * S-01 fix: LPM.h was misnamed — it contained MSTPCR, not LPM logic.
 *
 * Default clock after reset: MOCO 8 MHz, all dividers = /1.
 * CLK_Init() makes this state explicit (idempotent, safe to call early).
 */

/* -----------------------------------------------------------------------
 * Clock Generation Circuit (CGC) registers  (RA6M5 HW Manual §8)
 * Base: SYSC = 0x4001E000
 * ----------------------------------------------------------------------- */
#define SCKDIVCR    (*(volatile uint32_t *)(uintptr_t)(SYSC + 0x020U))  /* System Clock Division Control   */
#define SCKSCR      (*(volatile uint8_t  *)(uintptr_t)(SYSC + 0x026U))  /* System Clock Source Control     */
#define PLLCCR      (*(volatile uint16_t *)(uintptr_t)(SYSC + 0x028U))  /* PLL Clock Control               */
#define PLLCR       (*(volatile uint8_t  *)(uintptr_t)(SYSC + 0x02AU))  /* PLL Control                     */
#define PLLSR       (*(volatile uint8_t  *)(uintptr_t)(SYSC + 0x02CU))  /* PLL Status                      */
#define HOCOCR      (*(volatile uint8_t  *)(uintptr_t)(SYSC + 0x036U))  /* HOCO Control                    */
#define MOCOCR      (*(volatile uint8_t  *)(uintptr_t)(SYSC + 0x038U))  /* MOCO Control                    */
#define OSCSF       (*(volatile uint8_t  *)(uintptr_t)(SYSC + 0x03CU))  /* Oscillation Stabilization Flag  */

/* SCKSCR.CKSEL[2:0] clock source values */
#define SCKSCR_HOCO  0x00U   /* HOCO (up to 64 MHz) */
#define SCKSCR_MOCO  0x01U   /* MOCO (8 MHz, reset default) */
#define SCKSCR_LOCO  0x02U   /* LOCO (32 kHz) */
#define SCKSCR_MOSC  0x03U   /* Main clock oscillator */
#define SCKSCR_PLL   0x05U   /* PLL */

/* SCKDIVCR divider encoding (same for all fields) */
#define SCKDIV_1     0x0U
#define SCKDIV_2     0x1U
#define SCKDIV_4     0x2U
#define SCKDIV_8     0x3U
#define SCKDIV_16    0x4U
#define SCKDIV_32    0x5U
#define SCKDIV_64    0x6U

/* SCKDIVCR field bit positions */
#define SCKDIVCR_PCKDPOS   0U    /* PCLKD [2:0] */
#define SCKDIVCR_PCKBPOS   8U    /* PCLKB [10:8] */
#define SCKDIVCR_PCKAPOS  12U    /* PCLKA [14:12] */
#define SCKDIVCR_BCKPOS   16U    /* BCLK  [18:16] */
#define SCKDIVCR_ICKPOS   24U    /* ICLK  [26:24] */
#define SCKDIVCR_FCKPOS   28U    /* FCLK  [30:28] */

/* -----------------------------------------------------------------------
 * Module Stop Control registers  (RA6M5 HW Manual §10.2.4–10.2.7)
 *
 * MSTP block base: 0x4008_4000  (separate from SYSC — verified p.220)
 *   MSTPCRA offset 0x000  MSTPCRB offset 0x004
 *   MSTPCRC offset 0x008  MSTPCRD offset 0x00C
 *
 * bit=1: module clock stopped (reset default)
 * bit=0: module clock running  ← clear bit to enable
 *
 * NOTE: prior code used SYSC+0x700 = 0x4001E700 which is WRONG.
 *       That address is outside the MSTP block and writes were silently
 *       discarded → SCI7 never released from module stop → TDRE stuck at 0.
 * ----------------------------------------------------------------------- */
#define MSTP_BASE   0x40084000UL
#define MSTPCRA     (*(volatile uint32_t *)(uintptr_t)(MSTP_BASE + 0x000U))
#define MSTPCRB     (*(volatile uint32_t *)(uintptr_t)(MSTP_BASE + 0x004U))
#define MSTPCRC     (*(volatile uint32_t *)(uintptr_t)(MSTP_BASE + 0x008U))
#define MSTPCRD     (*(volatile uint32_t *)(uintptr_t)(MSTP_BASE + 0x00CU))

/* -----------------------------------------------------------------------
 * SCI channel enumeration
 * Used by CLK_ModuleStart_SCI().
 * MSTPCRB bit mapping: SCI0=bit31, SCI1=bit30, ..., SCI9=bit22
 * ----------------------------------------------------------------------- */
typedef enum {
    SCI0 = 0, SCI1 = 1, SCI2 = 2, SCI3 = 3, SCI4 = 4,
    SCI5 = 5, SCI6 = 6, SCI7 = 7, SCI8 = 8, SCI9 = 9
} SCI_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/*
 * CLK_Init — explicit MOCO 8 MHz clock configuration.
 * Call once from Reset_Handler before main().
 * Sets SCKDIVCR (all /1) and SCKSCR (MOCO) to known-good state.
 * Requires RWP unlock (handled internally).
 */
void CLK_Init(void);

/*
 * CLK_ModuleStart_SCI — release module stop for one SCI channel.
 * Replaces LPM_Unlock(). Wraps MSTPCRB write with PRCR unlock/lock.
 */
void CLK_ModuleStart_SCI(SCI_t peripheral);

#endif /* DRV_CLK_H */
