---
type: phase
phase: 7
status: todo
---

# Phase 7 — Add Timeout Protection to Busy-Wait Loops

Back to [[INDEX]] | Prev: [[PHASE_6]] | Next: [[PHASE_8]]

---

## Tasks

- [ ] P7-1 Define `DRV_TIMEOUT_TICKS` and `drv_status_t` in `drv_common.h`
- [ ] P7-2 Add timeout to `UART_SendChar` and `UART_ReceiveChar`
- [ ] P7-3 Add timeout to `I2C_Start`, `I2C_Stop`, `I2C_Transmit_Address`
- [ ] P7-4 Add timeout to `I2C_Master_Transmit_Data`, `I2C_Master_Receive_Data`
- [ ] P7-5 Add I2C bus recovery (9-clock SCL toggle sequence for stuck-low)

## Status: ⬜ TODO
