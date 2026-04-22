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

#endif /* DRV_COMMON_H */
