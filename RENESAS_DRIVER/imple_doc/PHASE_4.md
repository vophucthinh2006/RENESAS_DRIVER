---
type: phase
phase: 4
status: todo
---

# Phase 4 — Fix GPIO Driver

Back to [[INDEX]] | Prev: [[PHASE_3]] | Next: [[PHASE_5]]

Bugs to fix: [[BUGS]]

---

## Bugs Addressed

- BUG-10 — `GPIO_PortToIndex` returns `0` for invalid port (0 is PORT0)
- BUG-11 — `GPIO_MODE_t` has STM32 speed values, meaningless on RA6M5

## Tasks

- [ ] P4-1 Change `GPIO_PortToIndex` invalid return from `0` to `0xFF`
- [ ] P4-2 Add guard in GPIO functions: check `p == 0xFF` → return
- [ ] P4-3 Simplify `GPIO_MODE_t`: `GPIO_MODE_INPUT` / `GPIO_MODE_OUTPUT` only
- [ ] P4-4 Update all `GPIO_Config` call sites: `GPIO_MODE_OUT_10M` → `GPIO_MODE_OUTPUT`
- [ ] P4-5 Re-run GPIO unit tests — all must pass

## Status: ⬜ TODO
