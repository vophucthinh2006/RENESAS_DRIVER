---
type: phase
phase: 2
status: done
completed: 2026-04-22
bugs_fixed: [BUG-03, BUG-04, BUG-05, BUG-09, S-02]
---

# Phase 2 — Fix UART (SCI) Driver

Back to [[INDEX]] | Prev: [[PHASE_1]] | Next: [[PHASE_3]]

Bugs fixed: [[BUGS]]

---

## Bugs Addressed

- BUG-03 — `PCLKB=2MHz` wrong, BRR formula mismatches SEMR
- BUG-04 — Manual SSR TDRE clear harmful
- BUG-05 — Uninitialized pointer UB on default switch case
- BUG-09 — SEMR_ABCS/BGDM bit positions swapped
- S-02 — SCI.h 70-line flat macro block

## Tasks

- [x] P2-1 Verify SEMR bits: BGDM=bit6, ABCS=bit4 (RA6M5 HW Manual §34 Table 34.12)
- [x] P2-2 Update `#define PCLKB` to `8000000UL`
- [x] P2-3 Fix BRR formula: `PCLKB / (4 × baudrate) - 1` (matches BGDM=1, ABCS=1)
- [x] P2-4 Remove `*SSR &= ~SSR_TDRE` from `UART_SendChar`
- [x] P2-5 Add `default: return;` + channel index guard `if (n > 9U)` in all UART functions
- [x] P2-6 Refactor `SCI.h`: 70-line flat block replaced by `SCI_REG8(n, off)` pattern
- [x] P2-7 Rewrite `UART_Init`, `UART_SendChar`, `UART_ReceiveChar` using `SCI_xxx(n)` macros
- [x] P2-8 Fix `main.c`: relative includes, flat macros → `SCI_xxx(n)`, fix baud comment
- [ ] P2-9 Hardware verify: UART at 115200 baud (to be done by user on hardware)

## Changes Made

### `Driver/Include/SCI.h` — rewritten
- Removed 70-line flat macro block (`SCI0_SMR` … `SCI9_SEMR`)
- Added `SCI_REG8(n, off)` single accessor macro
- Added `SCI_SMR(n)`, `SCI_BRR(n)`, `SCI_SCR(n)`, `SCI_TDR(n)`, `SCI_SSR(n)`, `SCI_RDR(n)`, `SCI_SEMR(n)`
- Fixed `SEMR_BGDM = (1U << 6)` — was wrongly named ABCS at bit 6
- Fixed `SEMR_ABCS = (1U << 4)` — was wrongly named BGDM at bit 5
- Updated `PCLKB = 8000000UL` (was 2000000UL)
- Added `SSR_ORER`, `SSR_FER`, `SSR_PER` bit defines
- Explicit enum values: `UART0=0 … UART9=9` (enables direct cast to channel index)

### `Driver/Source/SCI.c` — rewritten (300 lines → 130 lines)
- `UART_Init`: switch replaced by `n = (uint8_t)uart; if (n > 9U) return;`
- BRR formula: `PCLKB / (4UL * baudrate) - 1U`
- Removed `*SSR = 0x00` write (was clobbering error flags)
- `UART_SendChar`: removed bad `*SSR &= ~SSR_TDRE` line; simplified to 3 lines
- `UART_ReceiveChar`: simplified to 3 lines
- All functions guard against invalid channel index

### `src/main.c` — updated
- Fixed: `#include "../Driver/Include/SCI.h"` → `#include "SCI.h"`
- Fixed: `#include "../Driver/Include/GPIO.h"` → `#include "GPIO.h"`
- Fixed: `SCI7_TDR`, `SCI7_SSR` → `SCI_TDR(7)`, `SCI_SSR(7)`
- Removed: `*SSR &= (uint8_t)(~(uint8_t)SSR_TDRE)` (BUG-04)
- Fixed: baud comment "~125000" → "~117647 actual (2.1% error)"
- Added timeout guard in `uart_self_test`

## BRR Calculation Summary

| PCLKB | Baudrate | BRR | Actual baud | Error |
|-------|----------|-----|-------------|-------|
| 8 MHz | 115200 | 16 | 117 647 | 2.1% ✅ |
| 8 MHz | 9600 | 207 | 9 615 | 0.16% ✅ |
| 8 MHz | 57600 | 33 | 58 824 | 2.1% ✅ |
| 8 MHz | 230400 | 7 | 250 000 | 8.5% ⚠️ marginal |

## Status: ✅ DONE
