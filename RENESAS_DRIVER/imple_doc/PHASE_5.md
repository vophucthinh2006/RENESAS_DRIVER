---
type: phase
phase: 5
status: todo
---

# Phase 5 — Rename & Reorganize Driver Files

Back to [[INDEX]] | Prev: [[PHASE_4]] | Next: [[PHASE_6]]

File map: [[FILES]]

---

## Tasks

- [ ] P5-1 Rename `LPM.h/.c` → `drv_clk.h/.c`; rename `LPM_Unlock` → `CLK_ModuleStart`
- [ ] P5-2 Rename `RWP.h/.c` → `drv_rwp.h/.c`
- [ ] P5-3 Rename `SCI.h/.c` → `drv_uart.h/.c`
- [ ] P5-4 Rename `IIC.h/.c` → `drv_i2c.h/.c`
- [ ] P5-5 Merge `CGC.h` into `drv_clk.h`; delete `CGC.h` and `CGC.c`
- [ ] P5-6 Update `CMakeLists.txt` and `cmake/GeneratedSrc.cmake`
- [ ] P5-7 Update all `#include` across `src/` and `Driver/`
- [ ] P5-8 Full clean rebuild — zero errors

## Status: ⬜ TODO
