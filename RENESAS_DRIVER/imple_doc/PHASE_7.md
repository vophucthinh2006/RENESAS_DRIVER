---
type: phase
phase: 7
status: done
completed: 2026-04-22
bugs_fixed: [S-05, S-06]
---

# Phase 7 — Add Timeout Protection to Busy-Wait Loops

Back to [[INDEX]] | Prev: [[PHASE_6]] | Next: [[PHASE_8]]

---

## Structural Issues Addressed

- S-05 — No timeout in any busy-wait loop
- S-06 — No I2C bus recovery for SCL/SDA stuck-low

## Tasks

- [x] P7-1 Add `DRV_TIMEOUT_TICKS 100000UL` and `drv_status_t` enum to `drv_common.h`
- [x] P7-2 Add timeout to `UART_SendChar` and `UART_ReceiveChar` in `drv_uart.c`
- [x] P7-3 Add timeout to `I2C_Start`, `I2C_Stop` in `drv_i2c.c`
- [x] P7-4 Add timeout to `I2C_Transmit_Address`, `I2C_Master_Transmit_Data`, `I2C_Master_Receive_Data`
- [x] P7-5 Implement `i2c_bus_recover()` — 9-clock SCL toggle recovery (MIPI I2C §3.1.16)

## Changes Made

### `Driver/Include/drv_common.h`
- Added `#define DRV_TIMEOUT_TICKS  100000UL` (~12 ms at 8 MHz MOCO, -O0)
- Added `drv_status_t` enum: `DRV_OK=0`, `DRV_TIMEOUT=1`, `DRV_ERR=2`

### `Driver/Source/drv_uart.c`
- `UART_SendChar`: countdown from `DRV_TIMEOUT_TICKS` in TDRE wait; on timeout, byte is dropped (no hang)
- `UART_ReceiveChar`: countdown in RDRF wait; on timeout, returns `0`
- Public API signatures unchanged

### `Driver/Source/drv_i2c.c`
- All 6 busy-wait loops now have `DRV_TIMEOUT_TICKS` countdown
- On timeout: void functions return early; uint8_t functions return `0` (failure)
- Added `i2c_bus_recover(I2C_t i2c)` — static internal function
- Added `i2c_bit_delay()` — ~10 μs at 8 MHz for recovery timing

## I2C Bus Recovery (S-06) — Implementation Detail

`i2c_bus_recover()` is called from `I2C_Start` when `BBSY` is stuck after timeout:

1. Disable RIIC (ICE=0, IICRST=0)
2. Configure SCL as GPIO open-drain output, SDA as GPIO floating input
3. Drive SCL high; toggle SCL 9 times, checking SDA after each clock
4. Issue manual STOP condition (SDA low→high while SCL high)
5. Reconfigure SCL/SDA as RIIC peripheral function (PMR=1, NCODR=1)
6. Soft-reset RIIC (IICRST=1→ICE=1→IICRST=0) to flush state machine

Ports: I2C0=P400/401, I2C1=P512/511, I2C2=P410/409 (same as `i2c_pin_config`).

**Limitation**: if SCL is physically stuck low (hardware fault), recovery is impossible in software. The caller will see a timeout again on the next I2C_Start call.

## Status: ✅ DONE
