# RCA_I2C_ACK_NACK

Tags: #done #firmware #i2c

Root cause analysis for broken ACK/NACK logic in I2C master receive path. Affected module: [[FW_I2C_Driver]]. Hardware reference: [[HW_RA6M5_RIIC]].

---

## Symptoms

- I2C reads returned garbage data from slave
- Last byte of multi-byte read did not send NACK, so slave kept driving SDA
- Bus stuck busy after read sequence

---

## Hardware Root Cause

RA6M5 RIIC ICMR3 register controls the ACK/NACK bit (ACKBT) sent after each received byte.

**Hardware protection mechanism:** ACKBT is write-protected by ACKWP (bit 3 of ICMR3). Writing ACKBT when ACKWP=0 has **no effect** — the write is silently ignored. ACKWP must be set to 1 first, then ACKBT written, then ACKWP cleared.

**Last byte index:** The NACK must be sent with the **last byte** (index `length - 1`), not the second-to-last byte. Sending NACK on the penultimate byte terminates the read one byte early, losing the final byte.

---

## Root Cause in Code

Two major compounded bugs were found in Master Receive Mode:

**1. Missing Dummy Read & Late ACKBT Configuration:**
The original `I2C_Master_Receive_Data` waited for `RDRF` to become 1 before setting `ACKBT` and reading `ICDRR`.
However, in the RA RIIC, when a read address (`SLA+R`) is sent and acknowledged, the RIIC transfers the address byte to `ICDRR` and sets `RDRF=1`. The SCL line is held low until a **dummy read** of `ICDRR` is performed to start clocking the actual data bytes.
Because the original code skipped the dummy read, the first byte read was actually the dummy address byte (e.g., `0x71`). Furthermore, the hardware automatically started receiving the real first data byte with `ACKBT=0` (sending an ACK). For a 1-byte read (like `aht20_read_status`), sending an ACK caused the AHT20 sensor to pull SDA low for a second byte, which prevented the STOP condition from being generated and hung the bus.

**2. I2C_Start Recovery Return Bug:**
When the bus hung, the next call to `I2C_Start` timed out on the `BBSY` flag and triggered `i2c_bus_recover()`. However, immediately after recovery, the function executed a `return;` instead of proceeding to set `ICCR2_ST`. This meant no START condition was generated, causing the subsequent `I2C_Transmit_Address` to timeout and return `AHT20_ERR_NACK`.

---

## Fix

```c
/* 1. In I2C_Start: change return to break after recovery */
i2c_bus_recover(i2c);
break;   /* Proceed to generate START */

/* 2. In I2C_Master_Receive_Data: dummy read and early ACKBT */
while (!(ICSR2(n) & ICSR2_RDRF)); /* Wait for address byte */

/* Set ACKBT for the first real byte BEFORE releasing SCL */
ICMR3(n) |= ICMR3_ACKWP;
if (length == 1U) { ICMR3(n) |= ICMR3_ACKBT; }
else              { ICMR3(n) &= (uint8_t)~ICMR3_ACKBT; }
ICMR3(n) &= (uint8_t)~ICMR3_ACKWP;

volatile uint8_t dummy = ICDRR(n); /* Dummy read starts clocking */

/* CORRECT: unlock first, write ACKBT, re-lock */
ICMR3(n) |= ICMR3_ACKWP;                   /* Step 1: unlock ACKBT     */
if (i == (uint8_t)(length - 1U))
    ICMR3(n) |= ICMR3_ACKBT;               /* last byte → NACK         */
else
    ICMR3(n) &= (uint8_t)~ICMR3_ACKBT;    /* all other bytes → ACK    */
ICMR3(n) &= (uint8_t)~ICMR3_ACKWP;        /* Step 3: re-lock ACKBT    */
```
