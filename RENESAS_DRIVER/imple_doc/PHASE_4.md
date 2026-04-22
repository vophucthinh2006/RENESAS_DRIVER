---
type: phase
phase: 4
status: done
completed: 2026-04-22
bugs_fixed: [BUG-10, BUG-11]
---

# Phase 4 — Fix GPIO Driver

Back to [[INDEX]] | Prev: [[PHASE_3]] | Next: [[PHASE_5]]

Bugs fixed: [[BUGS]]

---

## Bugs Addressed

- BUG-10 — `GPIO_PortToIndex` returns `0` (valid PORT0) for invalid input
- BUG-11 — `GPIO_MODE_t` has STM32-style speed values, meaningless on RA6M5

## Tasks

- [x] P4-1 `GPIO_PortToIndex`: return `GPIO_INVALID_PORT` (0xFF) for invalid port
- [x] P4-2 Add guard `if (p == GPIO_INVALID_PORT) return` in all 3 GPIO functions
- [x] P4-3 Simplify `GPIO_MODE_t`: `GPIO_MODE_INPUT=0`, `GPIO_MODE_OUTPUT=1` only
- [x] P4-4 Define `GPIO_INVALID_PORT 0xFFU` constant in GPIO.h
- [x] P4-5 Update `hal_entry.c`: `GPIO_MODE_OUT_10M` → `GPIO_MODE_OUTPUT`
- [x] P4-6 Update `main.c`: `GPIO_MODE_OUT_10M` → `GPIO_MODE_OUTPUT`
- [x] P4-7 Update `test_gpio.c`: invalid port test now expects `GPIO_INVALID_PORT` (0xFF)
- [x] P4-8 Re-run GPIO unit tests — all 3 pass

## Changes Made

### `Driver/Include/GPIO.h` — updated
- Removed `GPIO_MODE_OUT_10M`, `GPIO_MODE_OUT_2M`, `GPIO_MODE_OUT_50M` (STM32 copy-paste)
- Added `GPIO_MODE_INPUT = 0`, `GPIO_MODE_OUTPUT = 1`
- Added `#define GPIO_INVALID_PORT 0xFFU` as sentinel constant
- Added inline comments for all register macros
- Cleaned up bit-literal syntax: `0b0/0b1` → `0/1`

### `Driver/Source/GPIO.c` — updated
- `GPIO_PortToIndex`: `return 0U` → `return GPIO_INVALID_PORT` for invalid port
- `GPIO_Config`: added `if (p == GPIO_INVALID_PORT) { return; }`
- `GPIO_Write_Pin`: added `if (p == GPIO_INVALID_PORT) { return; }`
- `GPIO_Read_Pin`: added `if (p == GPIO_INVALID_PORT) { return 0U; }`

### `src/hal_entry.c` — updated
- `GPIO_MODE_OUT_10M` → `GPIO_MODE_OUTPUT`

### `src/main.c` — updated
- `GPIO_MODE_OUT_10M` → `GPIO_MODE_OUTPUT`

### `src/test/test_gpio.c` — updated
- Test `test_port_index_invalid_returns_zero` → `test_port_index_invalid_returns_sentinel`
- Expected value: `0U` → `GPIO_INVALID_PORT`
- Registered name updated to `gpio_port_index_invalid_sentinel`

## Status: ✅ DONE
