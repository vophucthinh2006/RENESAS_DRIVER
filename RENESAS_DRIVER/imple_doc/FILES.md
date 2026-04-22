---
type: file-map
last_updated: 2026-04-22
---

# Driver File Structure Map

Back to [[INDEX]]

---

## Current Structure (pre-fix)

**Driver/Include/**

- `CGC.h` — uses SYSC but no include → BUG-02 ✅ fixed [[PHASE_1]]
- `GPIO.h` — MODE enum wrong → BUG-11 ⬜ [[PHASE_4]]
- `IIC.h` — dead enums, misleading names → BUG-14 ⬜ [[PHASE_3]]
- `LPM.h` — misnamed; SYSC conflict → BUG-01, S-01, S-04 ✅ [[PHASE_1]]
- `RWP.h` — SYSC conflict → BUG-01, S-04 ✅ [[PHASE_1]]
- `SCI.h` — 70-line flat macro block → BUG-03, BUG-09, S-02 ⬜ [[PHASE_2]]
- `drv_common.h` — **NEW** single SYSC definition ✅ [[PHASE_1]]

**Driver/Source/**

- `CGC.c` — EMPTY STUB → S-03 ⬜ [[PHASE_6]]
- `GPIO.c` — invalid port returns 0 → BUG-10 ⬜ [[PHASE_4]]
- `IIC.c` — init seq wrong, ACK/NACK broken → BUG-06, BUG-07, BUG-08, BUG-12 ⬜ [[PHASE_3]]
- `LPM.c` — only covers SCI, not I2C → BUG-12 ⬜ [[PHASE_3]]
- `RWP.c` — OK, rename needed ⬜ [[PHASE_5]]
- `SCI.c` — PCLKB wrong, UB, bad SSR clear → BUG-03, BUG-04, BUG-05 ⬜ [[PHASE_2]]

**src/**

- `hal_entry.c` — duplicate delay_ms → BUG-13 ⬜ [[PHASE_8]]
- `main.c` — relative includes, dup delay_ms → BUG-13, BUG-15 ⬜ [[PHASE_8]]
- `startup.c` — OK
- `test/test_runner.c` — wrong fail counter → BUG-16 ⬜ [[PHASE_8]]

---

## Target Structure (after Phase 5)

**Driver/Include/**

- `drv_common.h` ✅ Created [[PHASE_1]]
- `drv_rwp.h` ⬜ [[PHASE_5]] (rename from RWP.h)
- `drv_clk.h` ⬜ [[PHASE_5]] (merge CGC.h + LPM.h)
- `drv_gpio.h` ⬜ [[PHASE_4]] + [[PHASE_5]]
- `drv_uart.h` ⬜ [[PHASE_2]] + [[PHASE_5]]
- `drv_i2c.h` ⬜ [[PHASE_3]] + [[PHASE_5]]

**Driver/Source/**

- `drv_rwp.c` ⬜ [[PHASE_5]]
- `drv_clk.c` ⬜ [[PHASE_6]] (implement CGC init)
- `drv_gpio.c` ⬜ [[PHASE_4]] + [[PHASE_5]]
- `drv_uart.c` ⬜ [[PHASE_2]] + [[PHASE_5]]
- `drv_i2c.c` ⬜ [[PHASE_3]] + [[PHASE_5]]

**src/**

- `utils.h` ⬜ [[PHASE_8]] (new — shared delay_ms)
- `hal_entry.c` ⬜ [[PHASE_8]]
- `main.c` ⬜ [[PHASE_8]]
- `test/test_runner.c` ⬜ [[PHASE_8]]
