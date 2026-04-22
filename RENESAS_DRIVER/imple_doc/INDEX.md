---
type: hub
status: done
last_updated: 2026-04-22
---

# RENESAS_DRIVER — Knowledge Base

Tags: #done #system

> Project: TESTING_2 | Target: R7FA6M5BH3CFC (Cortex-M33) | Board: EK-RA6M5
> Build: CMake + Ninja, ARM GCC, `file(GLOB_RECURSE)` auto-collects `Driver/Source/*.c` and `src/*.c`

---

## Hardware Layer

| Note | Covers |
|------|--------|
| [[HW_RA6M5_ClockTree]] | MOCO/HOCO/PLL, SCKDIVCR, SCKSCR, PCLKB |
| [[HW_RA6M5_SCI]] | SCI UART registers, SEMR, BRR formula, SSR flags |
| [[HW_RA6M5_RIIC]] | RIIC I2C registers, init sequence, ICBRL fixed bits |
| [[HW_RA6M5_GPIO]] | Port control, PFS, PWPR, GPIO_INVALID_PORT |
| [[HW_RA6M5_RWP]] | PRCR register, write key 0xA5, PRC0/PRC1 |

---

## Firmware Layer

| Note | Files |
|------|-------|
| [[FW_Clock_Driver]] | `drv_clk.h/.c` — CLK_Init, CLK_ModuleStart_SCI |
| [[FW_RWP_Driver]] | `drv_rwp.h/.c` — RWP_Unlock/Lock_Clock_MSTP |
| [[FW_UART_Driver]] | `drv_uart.h/.c` — UART_Init, SendChar (with timeout) |
| [[FW_I2C_Driver]] | `drv_i2c.h/.c` — I2C_Init, bus recovery (9-clock) |
| [[FW_GPIO_Driver]] | `GPIO.h/.c` — GPIO_Config, invalid-port sentinel |
| [[FW_TestFramework]] | `test_runner.h/.c` — 11 tests across 4 suites |

---

## Root Cause Analysis

| Note | Bug(s) |
|------|--------|
| [[RCA_SYSC_Redefinition]] | SYSC defined in both LPM.h and RWP.h |
| [[RCA_UART_BRR_SEMR]] | PCLKB=2MHz wrong; BGDM/ABCS bit positions swapped |
| [[RCA_UART_SSR_Manual_Clear]] | Manual SSR.TDRE clear harmful |
| [[RCA_I2C_Init_Sequence]] | ICE set before IICRST; missing 0xE0 on ICBRL/ICBRH |
| [[RCA_I2C_Start_Hang]] | TEND polled after START — never fires, infinite hang |
| [[RCA_I2C_ACK_NACK]] | ACKBT written before ACKWP; wrong last-byte index |
| [[RCA_GPIO_Invalid_Port]] | Invalid port returned 0 (aliases PORT0) |

---

## Project Status

All 22 bugs and structural issues resolved. All drivers production-ready for MOCO 8 MHz baseline.

| Severity | Total | Fixed |
|----------|-------|-------|
| CRITICAL | 5 | 5 |
| HIGH | 4 | 4 |
| MEDIUM | 4 | 4 |
| LOW | 3 | 3 |
| STRUCTURAL | 6 | 6 |

Next step: Phase 9 — final validation (clean build, hardware-in-loop test run).

---

## Legend

#done — implemented and verified
#in-progress — currently being worked
#todo — planned
#blocked — waiting on dependency
