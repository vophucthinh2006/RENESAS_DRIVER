#ifndef DRV_COMMON_H
#define DRV_COMMON_H

#include <stdint.h>

/*
 * drv_common.h — Shared base addresses and types for all RA6M5 drivers.
 *
 * This is the single source of truth for register base addresses that are
 * shared across multiple peripheral drivers (SYSC, etc.).
 * Include this header in every driver header instead of re-defining bases.
 */

/* System Control (SYSC) base address — RA6M5 HW manual §9 */
#define SYSC  0x4001E000UL

/* -----------------------------------------------------------------------
 * Timeout protection (S-05)
 * DRV_TIMEOUT_TICKS: iteration count for busy-wait loops.
 * At 8 MHz MOCO with -O0: ~1 tick ≈ 125 ns → 100000 ticks ≈ 12 ms.
 * All drivers use a countdown from this value; zero = timeout error.
 * ----------------------------------------------------------------------- */
#define DRV_TIMEOUT_TICKS  100000UL

/* -----------------------------------------------------------------------
 * Driver status codes — returned by timeout-aware driver functions.
 * ----------------------------------------------------------------------- */
typedef enum {
    DRV_OK      = 0,   /* Operation completed successfully */
    DRV_TIMEOUT = 1,   /* Busy-wait timed out             */
    DRV_ERR     = 2    /* General error (NACK, etc.)      */
} drv_status_t;

#endif /* DRV_COMMON_H */
