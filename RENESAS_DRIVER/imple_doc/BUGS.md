---
type: bug-tracker
last_updated: 2026-04-22
---

# Bug Tracker

Back to [[INDEX]]

---

## CRITICAL Bugs

| ID | File | Description | Status |
|----|------|-------------|--------|
| BUG-01 | `LPM.h:5`, `RWP.h:5` | `SYSC` macro defined twice → redefinition | ✅ Fixed — [[PHASE_1]] |
| BUG-02 | `CGC.h:6` | Uses `SYSC` without including it | ✅ Fixed — [[PHASE_1]] |
| BUG-03 | `SCI.h:118`, `SCI.c:194` | `PCLKB=2MHz` wrong; BRR formula mismatches SEMR | ✅ Fixed — [[PHASE_2]] |
| BUG-04 | `SCI.c:247` | Manual SSR TDRE clear is wrong/harmful | ✅ Fixed — [[PHASE_2]] |
| BUG-05 | `SCI.c:199-301` | Uninitialized pointer UB on default switch case | ✅ Fixed — [[PHASE_2]] |

## HIGH Bugs

| ID | File | Description | Status |
|----|------|-------------|--------|
| BUG-06 | `IIC.c:87-91` | I2C init: ICE set before IICRST, violates HW spec | ✅ Fixed — [[PHASE_3]] |
| BUG-07 | `IIC.c:113-114` | I2C_Start waits TEND after START → hangs forever | ✅ Fixed — [[PHASE_3]] |
| BUG-08 | `IIC.c:236-248` | ACK/NACK logic broken, ACKWP used incorrectly | ✅ Fixed — [[PHASE_3]] |
| BUG-09 | `SCI.h:120-121` | SEMR_ABCS/BGDM bit positions may be swapped | ✅ Fixed — [[PHASE_2]] |

## MEDIUM Bugs

| ID | File | Description | Status |
|----|------|-------------|--------|
| BUG-10 | `GPIO.c:10` | Invalid port returns `0` (valid PORT0), no sentinel | ✅ Fixed — [[PHASE_4]] |
| BUG-11 | `GPIO.h:57-62` | GPIO_MODE_t has STM32 speed values, meaningless on RA6M5 | ✅ Fixed — [[PHASE_4]] |
| BUG-12 | `IIC.c:13-26` | I2C bypasses LPM_Unlock, directly writes MSTPCRB | ✅ Fixed — [[PHASE_3]] |
| BUG-13 | `hal_entry.c`, `main.c` | `delay_ms` duplicated in two files | ⬜ [[PHASE_8]] |

## LOW Bugs

| ID | File | Description | Status |
|----|------|-------------|--------|
| BUG-14 | `IIC.h:36-56` | `I2C_PINCFG_t` and `I2C_ACK_t` declared but never used | ✅ Fixed — [[PHASE_3]] |
| BUG-15 | `main.c:12-13` | Relative include paths inconsistent with project | ✅ Fixed — [[PHASE_2]] |
| BUG-16 | `test_runner.c:29` | `g_test_fail_count` counts assertions, not test cases | ⬜ [[PHASE_8]] |

## Structural Issues

| ID | Description | Status |
|----|-------------|--------|
| S-01 | `LPM.h` misnamed — contains MSTPCR, not LPM logic | ✅ Fixed — [[PHASE_5]] |
| S-02 | `SCI.h` has 70+ flat macros (10 ch × 7 reg) | ✅ Fixed — [[PHASE_2]] |
| S-03 | `CGC.c` is empty stub — clock never configured | ✅ Fixed — [[PHASE_6]] |
| S-04 | `SYSC` defined in two files simultaneously | ✅ Fixed — [[PHASE_1]] |
| S-05 | No timeout in any busy-wait loop | ⬜ [[PHASE_7]] |
| S-06 | No I2C bus recovery for SCL/SDA stuck-low | ⬜ [[PHASE_7]] |
