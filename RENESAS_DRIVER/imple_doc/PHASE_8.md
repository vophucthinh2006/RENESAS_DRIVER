---
type: phase
phase: 8
status: todo
---

# Phase 8 — Clean Up src/ Layer

Back to [[INDEX]] | Prev: [[PHASE_7]] | Next: [[PHASE_9]]

---

## Tasks

- [ ] P8-1 Create `src/utils.h` with `static inline delay_ms()`; remove duplicates
- [ ] P8-2 Fix `main.c`: relative includes → `#include "drv_uart.h"` style
- [ ] P8-3 Fix `test_runner.c`: `g_test_fail_count` increments only once per failed test
- [ ] P8-4 Decide `main.c` fate: standalone or merge into test framework
- [ ] P8-5 Add UART and I2C test stubs

## Status: ⬜ TODO
