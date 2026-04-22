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

The original `I2C_Master_Receive_Data()` had two errors:

**Error 1 — ACKBT written before ACKWP set:**
```c
/* WRONG: original order */
ICMR3(n) |= ICMR3_ACKBT;    /* write ignored — ACKWP=0         */
ICMR3(n) |= ICMR3_ACKWP;    /* unlock came too late            */
```

**Error 2 — Wrong last-byte index:**
```c
if (i == (length - 2))       /* WRONG: sends NACK one byte early */
```

---

## Fix

```c
/* CORRECT: unlock first, write ACKBT, re-lock */
ICMR3(n) |= ICMR3_ACKWP;                   /* Step 1: unlock ACKBT     */
if (i == (uint8_t)(length - 1U))
    ICMR3(n) |= ICMR3_ACKBT;               /* last byte → NACK         */
else
    ICMR3(n) &= (uint8_t)~ICMR3_ACKBT;    /* all other bytes → ACK    */
ICMR3(n) &= (uint8_t)~ICMR3_ACKWP;        /* Step 3: re-lock ACKBT    */
```
