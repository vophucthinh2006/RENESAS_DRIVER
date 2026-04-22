#include "drv_i2c.h"
#include "drv_clk.h"
#include "drv_rwp.h"
#include "GPIO.h"

/* -----------------------------------------------------------------------
 * i2c_clock_init — release module stop for one RIIC channel.
 *
 * MSTPCRB bit mapping (RA6M5 HW manual §9.3.3):
 *   I2C0 → bit 9,  I2C1 → bit 8,  I2C2 → bit 7
 * ----------------------------------------------------------------------- */
static void i2c_clock_init(I2C_t i2c)
{
    RWP_Unlock_Clock_MSTP();

    switch (i2c)
    {
        case I2C0: MSTPCRB &= ~(1UL << 9U); break;
        case I2C1: MSTPCRB &= ~(1UL << 8U); break;
        case I2C2: MSTPCRB &= ~(1UL << 7U); break;
        default:   break;
    }

    RWP_Lock_Clock_MSTP();
}

/* -----------------------------------------------------------------------
 * i2c_pin_config — configure SCL/SDA pins for the selected RIIC channel.
 *
 * Pin mapping (EK-RA6M5 schematic, PSEL=0x07 for RIIC function):
 *   I2C0: SCL=P400, SDA=P401
 *   I2C1: SCL=P512, SDA=P511
 *   I2C2: SCL=P410, SDA=P409
 *
 * I2C pins require NCODR=1 (open-drain) and no pull-up (PCR=0).
 * ----------------------------------------------------------------------- */
static void i2c_pin_config(I2C_t i2c)
{
    uint8_t  scl_port, scl_pin;
    uint8_t  sda_port, sda_pin;
    const uint32_t psel = 0x07U;   /* PSEL = RIIC function */

    switch (i2c)
    {
        case I2C0: scl_port=4; scl_pin=0;  sda_port=4; sda_pin=1;  break;
        case I2C1: scl_port=5; scl_pin=12; sda_port=5; sda_pin=11; break;
        case I2C2: scl_port=4; scl_pin=10; sda_port=4; sda_pin=9;  break;
        default: return;
    }

    PWPR = 0x00U;   /* Step 1: clear B0WI  */
    PWPR = 0x40U;   /* Step 2: set  PFSWE  */

    /* SCL pin: peripheral (PMR=1), open-drain (NCODR=1), no pull (PCR=0) */
    PmnPFS(scl_port, scl_pin)  = PmnPFS_PSEL(psel);
    PmnPFS(scl_port, scl_pin) |= PmnPFS_PMR;
    PmnPFS(scl_port, scl_pin) |= PmnPFS_NCODR;
    PmnPFS(scl_port, scl_pin) &= ~PmnPFS_PCR;

    /* SDA pin: peripheral (PMR=1), open-drain (NCODR=1), no pull (PCR=0) */
    PmnPFS(sda_port, sda_pin)  = PmnPFS_PSEL(psel);
    PmnPFS(sda_port, sda_pin) |= PmnPFS_PMR;
    PmnPFS(sda_port, sda_pin) |= PmnPFS_NCODR;
    PmnPFS(sda_port, sda_pin) &= ~PmnPFS_PCR;

    PWPR = 0x00U;   /* Step 3: clear PFSWE */
    PWPR = 0x80U;   /* Step 4: set  B0WI   */
}

/* -----------------------------------------------------------------------
 * I2C_Init — initialise RIIC channel in master mode.
 *
 * Correct init sequence (RA6M5 HW manual §38.3):
 *   1. ICCR1 = 0x00          (ICE=0, IICRST=0  — disable first)
 *   2. ICCR1 |= IICRST        (assert reset while disabled)
 *   3. ICCR1 |= ICE           (enable while in reset)
 *   4. Configure ICBRL, ICBRH, ICMR registers
 *   5. ICCR1 &= ~IICRST       (release reset — peripheral starts)
 *
 * BRR formula (equal split between BRL and BRH):
 *   total_counts = (PCLKB / speed_hz) - 2
 *   ICBRL = ICBRH = ICBR_FIXED_BITS | (total_counts / 2)
 * ----------------------------------------------------------------------- */
void I2C_Init(I2C_t i2c, uint8_t pclkb_mhz, I2C_SPEED_t speed)
{
    uint8_t n = (uint8_t)i2c;
    if (n > 2U) { return; }

    i2c_clock_init(i2c);
    i2c_pin_config(i2c);

    uint32_t pclkb_hz   = (uint32_t)pclkb_mhz * 1000000UL;
    uint32_t total      = (pclkb_hz / (uint32_t)speed) - 2U;
    uint8_t  br         = (uint8_t)(total / 2U);

    /* Step 1: disable */
    ICCR1(n) = 0x00U;

    /* Step 2: assert reset (IICRST=1) while ICE=0 */
    ICCR1(n) |= ICCR1_IICRST;

    /* Step 3: enable (ICE=1) while still in reset */
    ICCR1(n) |= ICCR1_ICE;

    /* Step 4: configure bit rate and mode registers */
    ICBRL(n) = (uint8_t)(ICBR_FIXED_BITS | br);   /* bits[7:5]=111 required */
    ICBRH(n) = (uint8_t)(ICBR_FIXED_BITS | br);

    ICMR1(n) = 0x00U;
    ICMR2(n) = 0x00U;
    ICMR3(n) = 0x00U;

    ICFER(n) &= (uint8_t)~(1U << 0U);   /* SCLE=0: disable SCL sync circuit */

    /* Step 5: release reset — peripheral starts operating */
    ICCR1(n) &= (uint8_t)~ICCR1_IICRST;
}

/* -----------------------------------------------------------------------
 * I2C_Start — generate a START condition on the I2C bus.
 * ----------------------------------------------------------------------- */
void I2C_Start(I2C_t i2c)
{
    uint8_t n = (uint8_t)i2c;
    if (n > 2U) { return; }

    while (ICCR2(n) & ICCR2_BBSY) {}       /* wait: bus free  (BBSY=0) */
    ICCR2(n) |= ICCR2_ST;                   /* request START condition  */
    while (!(ICCR2(n) & ICCR2_BBSY)) {}    /* wait: bus busy  (BBSY=1) */
}

/* -----------------------------------------------------------------------
 * I2C_Stop — generate a STOP condition on the I2C bus.
 * ----------------------------------------------------------------------- */
void I2C_Stop(I2C_t i2c)
{
    uint8_t n = (uint8_t)i2c;
    if (n > 2U) { return; }

    ICCR2(n) |= ICCR2_SP;                          /* request STOP condition      */
    while (!(ICSR2(n) & ICSR2_STOP)) {}            /* wait: STOP flag set         */
    ICSR2(n) &= (uint8_t)~ICSR2_STOP;              /* clear STOP flag             */
    while (ICCR2(n) & ICCR2_BBSY) {}               /* wait: bus free (BBSY=0)     */
}

/* -----------------------------------------------------------------------
 * I2C_Transmit_Address — send 7-bit slave address + R/W bit.
 * Returns 1 on ACK, 0 on NACK.
 * ----------------------------------------------------------------------- */
uint8_t I2C_Transmit_Address(I2C_t i2c, uint8_t address, I2C_DIR_t dir)
{
    uint8_t n = (uint8_t)i2c;
    if (n > 2U) { return 0U; }

    while (!(ICSR2(n) & ICSR2_TDRE)) {}

    ICDRT(n) = (uint8_t)((address << 1U) | ((uint8_t)dir & 0x01U));

    while (!(ICSR2(n) & ICSR2_TEND)) {}

    if (ICSR2(n) & ICSR2_NACKF)
    {
        ICSR2(n) &= (uint8_t)~ICSR2_NACKF;
        return 0U;
    }

    return 1U;
}

/* -----------------------------------------------------------------------
 * I2C_Master_Transmit_Data — send data bytes to slave.
 * Returns 1 on success, 0 if NACK received.
 * ----------------------------------------------------------------------- */
uint8_t I2C_Master_Transmit_Data(I2C_t i2c, uint8_t *data, uint8_t length)
{
    uint8_t n = (uint8_t)i2c;
    uint8_t i;
    if (n > 2U) { return 0U; }

    for (i = 0U; i < length; i++)
    {
        while (!(ICSR2(n) & ICSR2_TDRE)) {}
        ICDRT(n) = data[i];
        while (!(ICSR2(n) & ICSR2_TEND)) {}

        if (ICSR2(n) & ICSR2_NACKF)
        {
            ICSR2(n) &= (uint8_t)~ICSR2_NACKF;
            I2C_Stop(i2c);
            return 0U;
        }
    }

    return 1U;
}

/* -----------------------------------------------------------------------
 * I2C_Master_Receive_Data — receive data bytes from slave.
 *
 * ACK/NACK sequencing (RA6M5 §38.3 master receive flow):
 *   - For all bytes except the last: send ACK  (ACKBT=0)
 *   - For the last byte:             send NACK (ACKBT=1)
 *   - ACKWP must be set BEFORE writing ACKBT, then cleared after.
 * Returns 1 on success.
 * ----------------------------------------------------------------------- */
uint8_t I2C_Master_Receive_Data(I2C_t i2c, uint8_t *data, uint8_t length)
{
    uint8_t n = (uint8_t)i2c;
    uint8_t i;
    if (n > 2U) { return 0U; }
    if (length == 0U) { return 1U; }

    for (i = 0U; i < length; i++)
    {
        while (!(ICSR2(n) & ICSR2_RDRF)) {}

        ICMR3(n) |= ICMR3_ACKWP;                   /* unlock ACKBT           */
        if (i == (uint8_t)(length - 1U))
        {
            ICMR3(n) |= ICMR3_ACKBT;               /* last byte → NACK       */
        }
        else
        {
            ICMR3(n) &= (uint8_t)~ICMR3_ACKBT;     /* other bytes → ACK      */
        }
        ICMR3(n) &= (uint8_t)~ICMR3_ACKWP;         /* lock ACKBT             */

        data[i] = ICDRR(n);   /* read byte — also clears RDRF */
    }

    I2C_Stop(i2c);

    return 1U;
}
