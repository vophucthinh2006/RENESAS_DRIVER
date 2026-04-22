---
type: phase
phase: 8
status: done
completed: 2026-04-22
bugs_fixed: [BUG-13, BUG-16]
---

# Phase 8 — Clean Up src/ Layer

Back to [[INDEX]] | Prev: [[PHASE_7]] | Next: [[PHASE_9]]

---

## Bugs Addressed

- BUG-13 — `delay_ms` duplicated in `hal_entry.c` and `main.c`
- BUG-16 — `g_test_fail_count` counted assertions, not test cases

## Tasks

- [x] P8-1 Create `src/utils.h` with `static inline delay_ms()`; remove duplicates from `hal_entry.c` and `main.c`
- [x] P8-2 Update includes: `main.c` already uses `drv_uart.h` (done Phase 5); added `utils.h`
- [x] P8-3 Fix `test_runner.c`: move `g_test_fail_count++` from `test_record_failure` to `test_run_all`
- [x] P8-4 `main.c` stays standalone — it is a separate UART smoke-test program, not merged into test framework
- [x] P8-5 Created `test_uart.c` (3 tests) and `test_i2c.c` (3 tests); updated `test_cases.h` and `hal_entry.c`

## Changes Made

### `src/utils.h` — NEW FILE
- `static inline delay_ms(uint32_t ms)`: calibrated for 8 MHz MOCO, -O0

### `src/hal_entry.c`
- Removed local `static void delay_ms()` definition
- Added `#include "utils.h"` (BUG-13 fix)
- Added `test_uart_register()` and `test_i2c_register()` calls

### `src/main.c`
- Removed local `static void delay_ms()` definition
- Added `#include "utils.h"` (BUG-13 fix)

### `src/test/test_runner.c`
- `test_record_failure()`: removed `g_test_fail_count++` (was counting per-assertion)
- `test_run_all()`: added `g_test_fail_count++` in the `else` branch (per-test count)
- Result: `test_run_all()` now returns number of **failed test cases**, not total assertion failures

### `src/test/test_cases.h`
- Added `void test_uart_register(void)` declaration
- Added `void test_i2c_register(void)` declaration

### `src/test/test_uart.c` — NEW FILE
- `test_uart7_scr_te_re_set`: verifies SCR has TE+RE set after `UART_Init`
- `test_uart7_tdre_ready`: verifies TDRE=1 (TX buffer empty) after init
- `test_uart7_brr_115200`: verifies BRR=16 (correct for 8 MHz / 4 / 115200 - 1)

### `src/test/test_i2c.c` — NEW FILE
- `test_i2c0_ice_set`: verifies ICCR1_ICE=1 after `I2C_Init`
- `test_i2c0_iicrst_cleared`: verifies IICRST=0 (reset released) after init
- `test_i2c0_bus_free`: verifies BBSY=0 (bus free) after init

## Test Count After Phase 8

| Suite | Tests |
|-------|-------|
| GPIO  | 3 |
| RWP   | 2 |
| UART  | 3 |
| I2C   | 3 |
| **Total** | **11** |

## Status: ✅ DONE
