# FW_Clock_Driver

Tags: #done #firmware #clock

Clock and module stop control driver for RA6M5. Files: `Driver/Include/drv_clk.h`, `Driver/Source/drv_clk.c`. Hardware constraints: [[HW_RA6M5_ClockTree]]. Write protection: [[FW_RWP_Driver]]. Formerly split across `CGC.h` (misnamed) and `LPM.h` (misnamed — contained MSTPCR, not LPM). Reorganized in Phase 5.

---

## API

```c
void CLK_Init(void);
void CLK_ModuleStart_SCI(SCI_t peripheral);
```

---

## CLK_Init

Writes the reset-default clock state explicitly to guarantee a known configuration. Called from `Reset_Handler` in `startup.c` before `main()`.

```c
void CLK_Init(void)
{
    RWP_Unlock_Clock_MSTP();
    SCKDIVCR = 0x00000000;    /* all dividers = /1 */
    SCKSCR   = SCKSCR_MOCO;  /* source = MOCO 8 MHz */
    RWP_Lock_Clock_MSTP();
}
```

Result: ICLK = PCLKB = PCLKA = BCLK = FCLK = 8 MHz. See [[HW_RA6M5_ClockTree]].

This is idempotent — safe to call multiple times. No oscillator stabilization wait needed for MOCO (already running at reset).

---

## CLK_ModuleStart_SCI

Clears the module stop bit for one SCI channel. Replaces the misnamed `LPM_Unlock()`. Wraps the MSTPCRB write with PRCR unlock/lock via [[FW_RWP_Driver]].

MSTPCRB mapping: SCI0=bit31, SCI1=bit30, ..., SCI9=bit22. Clearing the bit enables the module clock.

Called from `uart_clock_init()` (static, inside `drv_uart.c`) during `UART_Init`.

---

## MSTPCR Registers

Located at SYSC+0x700 (MSTPCRB), +0x704 (MSTPCRC), +0x708 (MSTPCRD). Hardware detail: [[HW_RA6M5_ClockTree]] §9.3.3.

RIIC module stop is also released here (MSTPCRB bits 9/8/7), by `i2c_clock_init()` (static, inside `drv_i2c.c`).

---

## File Structure Note

`drv_clk.h` merged what were previously two misnamed headers:
- `CGC.h` — contained clock generation registers
- `LPM.h` — contained MSTPCR + SCI_t enum (no LPM logic at all)

Old headers are now forwarding stubs: `#include "drv_clk.h"`.
