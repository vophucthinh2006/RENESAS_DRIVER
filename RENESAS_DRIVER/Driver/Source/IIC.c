#include "IIC.h"
#include "LPM.h"
#include "RWP.h"
#include "GPIO.h"

void I2C_Clock_Init(I2C_t I2C_)
{
    RWP_Unlock_Clock_MSTP();

    switch (I2C_)
    {
        case I2C0:
            MSTPCRB &= ~(1UL << 9);
            break;

        case I2C1:
            MSTPCRB &= ~(1UL << 8);
            break;

        case I2C2:
            MSTPCRB &= ~(1UL << 7);
            break;

        default:
            break;
    }

    RWP_Lock_Clock_MSTP();
}
void I2C_PinConfig(I2C_t I2C_){
    uint8_t scl_port, scl_pin;
    uint8_t sda_port, sda_pin;
    uint32_t psel = 0x07;

    switch (I2C_){
        case I2C0:
            scl_port = 4; scl_pin = 0;
            sda_port = 4; sda_pin = 1;
            break;
        case I2C1:
            scl_port = 5; scl_pin = 12;
            sda_port = 5; sda_pin = 11;
            break;
        case I2C2:
            scl_port = 4; scl_pin = 10;
            sda_port = 4; sda_pin = 9;
            break;
        default:return;
    }
    PWPR = 0x00U;    /* Step 1: clear B0WI  */
    PWPR = 0x40U;    /* Step 2: set  PFSWE  */

    PmnPFS(scl_port, scl_pin) = (psel << 8);
    PmnPFS(scl_port, scl_pin) |= PmnPFS_PMR;
    PmnPFS(scl_port, scl_pin) |= PmnPFS_NCODR;
    PmnPFS(scl_port, scl_pin) &= ~(PmnPFS_PCR);

    PmnPFS(sda_port, sda_pin) = (psel << 8);
    PmnPFS(sda_port, sda_pin) |= PmnPFS_PMR;
    PmnPFS(sda_port, sda_pin) |= PmnPFS_NCODR;
    PmnPFS(sda_port, sda_pin) &= ~(PmnPFS_PCR);

    PWPR = 0x00U;    /* Step 3: clear PFSWE */
    PWPR = 0x80U;    /* Step 4: set  B0WI   */
}
void I2C_Init(
    I2C_t I2C_,
    uint8_t clock_frequency_MHz,
    I2C_SPEED_t speed_kHz
){
    uint8_t n;

    switch (I2C_){
        case I2C0: n = 0; break;
        case I2C1: n = 1; break;
        case I2C2: n = 2; break;
        default: return;
    }

    I2C_Clock_Init(I2C_);
    I2C_PinConfig(I2C_);

    uint32_t PCLKB = clock_frequency_MHz * 1000000UL;
    uint32_t total = (PCLKB / speed_kHz) - 2;
    uint8_t br = (uint8_t)(total / 2);

    ICCR1(n) = 0x00;
    ICCR1(n) |= (uint8_t)(1U << 7);  /* ICE = 1, I2C enable */
    ICCR1(n) |= (uint8_t)(1U << 6);  /* IICRST = 1, internal reset */

    ICCR1(n) &= (uint8_t)~(1U << 6); /* IICRST = 0, release reset */

    ICBRL(n) = br;
    ICBRH(n) = br;

    ICMR1(n) = 0x00;
    ICMR2(n) = 0x00;
    ICMR3(n) = 0x00;

    ICFER(n) &= (uint8_t)~(1U << 0);  /* SCLE = 0, disable SCL synchronous circuit */
}
void I2C_Start(I2C_t I2C_){
    uint8_t n;

    switch (I2C_){
        case I2C0: n = 0; break;
        case I2C1: n = 1; break;
        case I2C2: n = 2; break;
        default: return;
    }
    while (ICCR2(n) & (1 << 7));
    ICCR2(n) |= (1 << 1);
    while (!(ICCR2(n) & (1 << 7)));
    while (!(ICSR2(n) & (1 << 6)));
}
void I2C_Stop(I2C_t I2C_)
{
    uint8_t n;

    switch (I2C_)
    {
        case I2C0: n = 0; break;
        case I2C1: n = 1; break;
        case I2C2: n = 2; break;
        default: return;
    }

    /* 1. Generate STOP */
    ICCR2(n) |= (1 << 3);   // SP = 1

    /* 2. Wait transmit end */
    while (!(ICSR2(n) & (1 << 6)));  // TEND = 1

    /* 3. Wait bus free */
    while (ICCR2(n) & (1 << 7));     // BBSY = 0
}
uint8_t I2C_Transmit_Address(
    I2C_t I2C_,
    uint8_t address,
    I2C_LSB_t write_read
)
{
    uint8_t n;
    uint8_t data;

    switch (I2C_)
    {
        case I2C0: n = 0; break;
        case I2C1: n = 1; break;
        case I2C2: n = 2; break;
        default: return 0;
    }

    /* 1. Wait transmit ready */
    while (!(ICSR2(n) & (1 << 7)));   // TDRE = 1

    /* 2. Send address */
    data = (address << 1) | (write_read & 0x01);
    ICDRT(n) = data;

    /* 3. Wait transmission complete */
    while (!(ICSR2(n) & (1 << 6)));   // TEND = 1

    /* 4. Check NACK */
    if (ICSR2(n) & (uint8_t)(1U << 4))          // NACKF = 1
    {
        ICSR2(n) &= (uint8_t)~(1U << 4);        // clear NACKF
        return 0;                     // FAIL
    }

    return 1;                         // SUCCESS
}
uint8_t I2C_Master_Transmit_Data(
    I2C_t I2C_,
    uint8_t *data,
    uint8_t data_length
)
{
    uint8_t n;
    uint8_t i;

    switch (I2C_)
    {
        case I2C0: n = 0; break;
        case I2C1: n = 1; break;
        case I2C2: n = 2; break;
        default: return 0;
    }

    for (i = 0; i < data_length; i++)
    {
        /* 1. Wait transmit ready */
        while (!(ICSR2(n) & (1 << 7)));   // TDRE = 1

        /* 2. Write data */
        ICDRT(n) = data[i];

        /* 3. Wait transmission complete */
        while (!(ICSR2(n) & (1 << 6)));   // TEND = 1

        /* 4. Check NACK */
        if (ICSR2(n) & (uint8_t)(1U << 4))
        {
            ICSR2(n) &= (uint8_t)~(1U << 4);        // clear NACK
            I2C_Stop(I2C_);
            return 0;                     // FAIL
        }
    }

    return 1; // SUCCESS
}
uint8_t I2C_Master_Receive_Data(
    I2C_t I2C_,
    uint8_t *data,
    uint8_t data_length
)
{
    uint8_t n;
    uint8_t i;

    switch (I2C_)
    {
        case I2C0: n = 0; break;
        case I2C1: n = 1; break;
        case I2C2: n = 2; break;
        default: return 0;
    }

    if (data_length == 0) return 1;

    for (i = 0; i < data_length; i++)
    {
        /* Wait for receive data full */
        while (!(ICSR2(n) & (uint8_t)(1U << 5)));  /* RDRF = 1 */

        ICMR3(n) |= (uint8_t)(1U << 4);  /* ACKBT = 1, set ACK bit */

        if (i == (data_length - 2))
        {
            ICMR3(n) |= (uint8_t)(1U << 3);  /* ACKWP = 1, wait = 1 */
        }
        else
        {
            ICMR3(n) &= (uint8_t)~(1U << 3); /* ACKWP = 0, wait = 0 */
        }

        ICMR3(n) &= (uint8_t)~(1U << 4);  /* ACKBT = 0, send ACK/NACK */

        data[i] = ICDRR(n);
    }

    /* Generate STOP condition */
    ICCR2(n) |= (1 << 3);  /* SP = 1, generate stop */

    return 1;  /* SUCCESS */
}
