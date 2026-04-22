# FW_I2C_Driver

Tags: #done #firmware #i2c

I2C master driver for RA6M5 RIIC peripheral. Files: `Driver/Include/drv_i2c.h`, `Driver/Source/drv_i2c.c`. Hardware constraints: [[HW_RA6M5_RIIC]]. Bug fixes applied: [[RCA_I2C_Init_Sequence]], [[RCA_I2C_Start_Hang]], [[RCA_I2C_ACK_NACK]].

---

## API

```c
void    I2C_Init(I2C_t i2c, uint8_t pclkb_mhz, I2C_SPEED_t speed);
void    I2C_Start(I2C_t i2c);
void    I2C_Stop(I2C_t i2c);
uint8_t I2C_Transmit_Address(I2C_t i2c, uint8_t address, I2C_DIR_t dir);
uint8_t I2C_Master_Transmit_Data(I2C_t i2c, uint8_t *data, uint8_t length);
uint8_t I2C_Master_Receive_Data(I2C_t i2c, uint8_t *data, uint8_t length);
```

Return values (uint8_t functions): 1 = success, 0 = NACK or timeout.

All busy-wait loops protected by `DRV_TIMEOUT_TICKS`. See `drv_common.h`.

---

## Initialization Sequence

Mandated by hardware — see [[HW_RA6M5_RIIC]] §38.3:

```c
ICCR1(n) = 0x00;           /* Step 1: disable (ICE=0, IICRST=0) */
ICCR1(n) |= ICCR1_IICRST;  /* Step 2: assert reset (while ICE=0) */
ICCR1(n) |= ICCR1_ICE;     /* Step 3: enable (while in reset)    */
ICBRL(n) = 0xE0U | br;     /* Step 4: configure bit rate         */
ICBRH(n) = 0xE0U | br;     /*   0xE0 = ICBR_FIXED_BITS required  */
ICMR1(n) = 0x00;           /* Step 4: configure mode             */
ICCR1(n) &= ~ICCR1_IICRST; /* Step 5: release reset              */
```

`ICBR_FIXED_BITS = 0xE0`: hardware requires ICBRL/ICBRH bits[7:5]=111. See [[HW_RA6M5_RIIC]].

---

## Bus Recovery (S-06)

`i2c_bus_recover()` — static, called from `I2C_Start` when BBSY is stuck after timeout:

1. Disable RIIC (`ICCR1 = 0x00`)
2. Configure SCL/SDA as GPIO (open-drain output / floating input)
3. Toggle SCL 9 times; stop early if SDA is released by slave
4. Issue manual STOP condition (SDA low→high while SCL high)
5. Reconfigure pins as RIIC peripheral (`i2c_pin_config`)
6. Soft-reset RIIC (IICRST=1 → ICE=1 → IICRST=0)

GPIO bit-bang uses [[FW_GPIO_Driver]] API. Pin assignments per channel from [[HW_RA6M5_RIIC]].

Limitation: if SCL is physically stuck low (hardware fault), software recovery cannot help.

---

## ACK/NACK Receive Sequencing

Per [[HW_RA6M5_RIIC]] §38.3 master receive flow:

```c
ICMR3(n) |= ICMR3_ACKWP;                  /* unlock ACKBT write   */
if (i == length - 1)
    ICMR3(n) |= ICMR3_ACKBT;              /* last byte → NACK     */
else
    ICMR3(n) &= ~ICMR3_ACKBT;             /* other bytes → ACK    */
ICMR3(n) &= ~ICMR3_ACKWP;                 /* re-lock              */
```

ACKWP must be set before ACKBT — writing ACKBT without ACKWP=1 has no effect. See [[RCA_I2C_ACK_NACK]].

---

## Test Coverage

`src/test/test_i2c.c` (hardware integration, requires powered target):
- `i2c0_ice_set`: ICCR1_ICE=1 after I2C_Init
- `i2c0_iicrst_cleared`: IICRST=0 after init
- `i2c0_bus_free`: BBSY=0 after init
