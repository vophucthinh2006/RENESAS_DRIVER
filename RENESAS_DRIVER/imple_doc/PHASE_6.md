---
type: phase
phase: 6
status: todo
---

# Phase 6 — Implement CGC Clock Initialization

Back to [[INDEX]] | Prev: [[PHASE_5]] | Next: [[PHASE_7]]

---

## Tasks

- [ ] P6-1 Decide clock option (MOCO 8MHz / HOCO / PLL); document choice
- [ ] P6-2 Implement `CLK_Init()` in `drv_clk.c`
- [ ] P6-3 Call `CLK_Init()` from `Reset_Handler` before peripheral init
- [ ] P6-4 Update `#define PCLKB` to match selected config exactly
- [ ] P6-5 Update `delay_ms` calibration if clock frequency changes

## Status: ⬜ TODO
