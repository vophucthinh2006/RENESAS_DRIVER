---
type: phase
phase: 5
status: done
completed: 2026-04-22
bugs_fixed: [S-01]
---

# Phase 5 — Rename & Reorganize Driver Files

Back to [[INDEX]] | Prev: [[PHASE_4]] | Next: [[PHASE_6]]

File map: [[FILES]]

---

## Structural Issue Addressed

- S-01 — `LPM.h` was misnamed: it contained MSTPCR (module stop), not LPM logic

## Tasks

- [x] P5-1 Create `drv_clk.h/.c` — merged CGC.h (clock registers) + LPM.h (MSTPCR + SCI_t); renamed `LPM_Unlock` → `CLK_ModuleStart_SCI`
- [x] P5-2 Create `drv_rwp.h/.c` — from RWP.h/.c (PRCR register write protection)
- [x] P5-3 Create `drv_uart.h/.c` — from SCI.h/.c; `SCI_Clock_Init` made static, calls `CLK_ModuleStart_SCI`
- [x] P5-4 Create `drv_i2c.h/.c` — from IIC.h/.c; `I2C_Clock_Init` made static, uses `drv_clk.h` for MSTPCRB
- [x] P5-5 Stub old headers as forwarding includes (backward compat); stub old .c files empty (prevent duplicate symbols with GLOB_RECURSE)
- [x] P5-6 No CMakeLists.txt changes needed — `file(GLOB_RECURSE)` auto-picks up new `drv_*.c` files
- [x] P5-7 Updated `#include` in src/ files: `main.c` → `drv_uart.h`, `test_rwp.c` → `drv_rwp.h`
- [x] P5-8 `LPM.h` forwarding include adds `#define LPM_Unlock(p) CLK_ModuleStart_SCI(p)` for backward compat

## Files Created

| New file | Replaces |
|----------|---------|
| `Driver/Include/drv_clk.h` | `CGC.h` + `LPM.h` |
| `Driver/Source/drv_clk.c` | `CGC.c` + `LPM.c` |
| `Driver/Include/drv_rwp.h` | `RWP.h` |
| `Driver/Source/drv_rwp.c` | `RWP.c` |
| `Driver/Include/drv_uart.h` | `SCI.h` |
| `Driver/Source/drv_uart.c` | `SCI.c` |
| `Driver/Include/drv_i2c.h` | `IIC.h` |
| `Driver/Source/drv_i2c.c` | `IIC.c` |

## Files Stubbed (old → forwarding)

- `CGC.h` → `#include "drv_clk.h"`
- `LPM.h` → `#include "drv_clk.h"` + `#define LPM_Unlock(p) CLK_ModuleStart_SCI(p)`
- `RWP.h` → `#include "drv_rwp.h"`
- `SCI.h` → `#include "drv_uart.h"`
- `IIC.h` → `#include "drv_i2c.h"`
- `CGC.c`, `LPM.c`, `RWP.c`, `SCI.c`, `IIC.c` → empty comment stubs

## Status: ✅ DONE
