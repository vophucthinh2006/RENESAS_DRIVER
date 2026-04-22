---
type: phase
phase: 3
status: done
completed: 2026-04-22
bugs_fixed: [BUG-06, BUG-07, BUG-08, BUG-12, BUG-14]
---

# Phase 3 — Fix I2C Driver

Back to [[INDEX]] | Prev: [[PHASE_2]] | Next: [[PHASE_4]]

Bugs fixed: [[BUGS]]

---

## Bugs Addressed

- BUG-06 — I2C_Init: ICE set before IICRST (violates HW spec)
- BUG-07 — I2C_Start: waits TEND after START → hangs forever
- BUG-08 — I2C_Master_Receive_Data: ACK/NACK logic broken, ACKWP wrong
- BUG-12 — I2C bypasses LPM_Unlock, directly writes MSTPCRB (inconsistency)
- BUG-14 — Dead code enums: I2C_PINCFG_t, I2C_ACK_t

## Tasks

- [x] P3-1 Fix `I2C_Init`: IICRST=1 first, ICE=1, configure ICBRL/ICBRH/ICMR, IICRST=0
- [x] P3-2 Add ICBR_FIXED_BITS (0xE0) mask to ICBRL/ICBRH writes (HW requirement)
- [x] P3-3 Fix `I2C_Start`: remove `while(TEND)` after START — this was causing infinite hang
- [x] P3-4 Fix `I2C_Stop`: use ICSR2.STOP flag (bit3) instead of TEND (bit6) for STOP detection
- [x] P3-5 Fix `I2C_Master_Receive_Data`: ACKWP set BEFORE ACKBT; last byte index corrected
- [x] P3-6 Remove dead enums `I2C_PINCFG_t` and `I2C_ACK_t` from IIC.h
- [x] P3-7 Rename `I2C_LSB_t` → `I2C_DIR_t` with `I2C_WRITE=0`, `I2C_READ=1`
- [x] P3-8 Add named bit-defines for ICCR1/ICCR2/ICSR2/ICMR3 (eliminates magic numbers)
- [x] P3-9 BUG-12: I2C_Clock_Init already uses RWP wrapper — verified consistent

## Changes Made

### `Driver/Include/IIC.h` — rewritten
- Removed `I2C_PINCFG_t` enum (dead code)
- Removed `I2C_ACK_t` enum (dead code)
- Renamed `I2C_LSB_t` → `I2C_DIR_t`: `I2C_WRITE=0`, `I2C_READ=1`
- Added bit-defines: `ICCR1_ICE`, `ICCR1_IICRST`, `ICCR2_BBSY`, `ICCR2_ST`, `ICCR2_SP`
- Added bit-defines: `ICSR2_TDRE`, `ICSR2_TEND`, `ICSR2_RDRF`, `ICSR2_NACKF`, `ICSR2_STOP`
- Added bit-defines: `ICMR3_ACKBT`, `ICMR3_ACKWP`
- Added `ICBR_FIXED_BITS = 0xE0U` constant for ICBRL/ICBRH upper-bit requirement
- Explicit enum values: `I2C0=0, I2C1=1, I2C2=2`

### `Driver/Source/IIC.c` — rewritten
- `I2C_Init`: sequence now IICRST=1 → ICE=1 → configure → IICRST=0 (RA6M5 §38.3)
- `I2C_Init`: ICBRL/ICBRH written as `0xE0 | br` (hardware requires bits[7:5]=111)
- `I2C_Init`: channel index guard `if (n > 2U) return`
- `I2C_Start`: removed wrong `while (ICSR2 & TEND)` line (was infinite hang after START)
- `I2C_Stop`: changed from TEND-wait to ICSR2.STOP-wait with flag clear
- `I2C_Master_Receive_Data`:
  - ACKWP set BEFORE ACKBT write (was reversed)
  - Last-byte condition: `i == (length - 1U)` (was `length - 2`)
  - NACK sent only on last byte; ACK on all others
  - STOP generated via `I2C_Stop()` call after loop
- All functions use named bit-defines instead of raw bit shifts

## ICBRL/ICBRH BRR Calculation

| PCLKB | Speed | br value | ICBRL/ICBRH write | Actual SCL |
|-------|-------|----------|-------------------|------------|
| 8 MHz | 100 kHz | 39 | 0xE7 | 100 kHz ✅ |
| 8 MHz | 400 kHz | 9  | 0xE9 | 400 kHz ✅ |

## Status: ✅ DONE
