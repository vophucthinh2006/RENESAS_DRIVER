---
type: phase
phase: 6
status: done
completed: 2026-04-22
bugs_fixed: [S-03]
---

# Phase 6 — Implement CGC Clock Initialization

Back to [[INDEX]] | Prev: [[PHASE_5]] | Next: [[PHASE_7]]

---

## Structural Issue Addressed

- S-03 — `CGC.c` was an empty stub; clock was never explicitly configured

## Tasks

- [x] P6-1 Clock option chosen: **MOCO 8 MHz explicit** (safest, no frequency change, idempotent)
- [x] P6-2 Implement `CLK_Init()` in `drv_clk.c`
- [x] P6-3 Call `CLK_Init()` from `Reset_Handler` in `startup.c` (step 5, before `main()`)
- [x] P6-4 `#define PCLKB 8000000UL` in `drv_uart.h` — confirmed correct (MOCO 8 MHz, /1 divider)
- [x] P6-5 `delay_ms` calibration unchanged — still calibrated for 8 MHz

## Clock Configuration

| Parameter | Value |
|-----------|-------|
| Source | MOCO (8 MHz, reset default) |
| ICLK | 8 MHz (ICK = /1) |
| PCLKB | 8 MHz (PCKB = /1) → matches `PCLKB` macro |
| PCLKA | 8 MHz (PCKA = /1) |
| FCLK | 8 MHz (FCK = /1) |
| Register state | SCKDIVCR = 0x00000000, SCKSCR = 0x01 |

## Design Decision

Strategy: make reset-default clock state **explicit** rather than assuming it.
- `CLK_Init()` writes SCKDIVCR = all /1 and SCKSCR = MOCO (same as reset default)
- Idempotent: safe to call multiple times
- No wait-for-stable needed (MOCO already running at reset)
- Future migration path documented in `drv_clk.c` comments (HOCO 64 MHz)

## Changes Made

### `Driver/Include/drv_clk.h`
- Added `CLK_Init()` declaration
- Added `SCKSCR_xxx` constants, `SCKDIV_x` constants, `SCKDIVCR_xxxPOS` field positions

### `Driver/Source/drv_clk.c`
- Implemented `CLK_Init()`: RWP_Unlock → SCKDIVCR = 0 → SCKSCR = MOCO → RWP_Lock

### `src/startup.c`
- Added `#include "drv_clk.h"`
- Added `CLK_Init()` call as step 5 in `Reset_Handler`, before `main()`

## Status: ✅ DONE
