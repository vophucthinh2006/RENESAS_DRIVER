---
type: phase
phase: 3
status: todo
---

# Phase 3 — Fix I2C Driver

Back to [[INDEX]] | Prev: [[PHASE_2]] | Next: [[PHASE_4]]

Bugs to fix: [[BUGS]]

---

## Bugs Addressed

- BUG-06 — I2C_Init: ICE set before IICRST (violates HW spec)
- BUG-07 — I2C_Start: waits TEND after START → hangs forever
- BUG-08 — I2C_Master_Receive_Data: ACK/NACK logic broken
- BUG-12 — I2C bypasses LPM_Unlock, directly writes MSTPCRB
- BUG-14 — Dead code enums: I2C_PINCFG_t, I2C_ACK_t

## Tasks

- [ ] P3-1 Fix `I2C_Init` sequence: IICRST=1 first, ICE=1, configure, IICRST=0
- [ ] P3-2 Fix `I2C_Start`: remove incorrect `while(TEND)` after START
- [ ] P3-3 Fix `I2C_Master_Receive_Data`: ACKWP before ACKBT, last-byte NACK correct
- [ ] P3-4 Extend `LPM_Unlock` to accept I2C peripherals
- [ ] P3-5 Remove unused `I2C_PINCFG_t` and `I2C_ACK_t` enums
- [ ] P3-6 Rename `I2C_LSB_t` → `I2C_DIR_t` with `I2C_WRITE=0`, `I2C_READ=1`
- [ ] P3-7 Hardware verify: I2C read/write with slave device

## Status: ⬜ TODO
