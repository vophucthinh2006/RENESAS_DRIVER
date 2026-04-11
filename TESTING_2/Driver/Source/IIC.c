#include "IIC.h"

void I2C_Clock_Init(I2C_t I2C_)
{
    RWP_Unlock_Clock_MSTP();

    switch (I2C_)
    {
        case I2C0:
            MSTPCRB &= ~(1 << 9);
            break;

        case I2C1:
            MSTPCRB &= ~(1 << 8);
            break;

        case I2C2:
            MSTPCRB &= ~(1 << 7);
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
    PWPR = 0x00;

    PmnPFS(scl_port, scl_pin) = (psel);
    PmnPFS(scl_port, scl_pin) |= (1 << 16);
    PmnPFS(scl_port, scl_pin) |= (1 << 6);
    PmnPFS(scl_port, scl_pin) &= ~(1 << 4);

    PmnPFS(sda_port, sda_pin) = (psel);
    PmnPFS(sda_port, sda_pin) |= (1 << 16);
    PmnPFS(sda_port, sda_pin) |= (1 << 6);
    PmnPFS(sda_port, sda_pin) &= ~(1 << 4);

    PWPR = 0x80;
}
void I2C_Init(
    I2C_t I2C_,
    uint8_t clock_frequency_MHz,
    I2C_SPEED_t speed_kHz
){
    uint8_t n;
    uint32_t PCLKB;
    uint8_t br;

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
    ICCR1(n) |= (1 << 7);

    ICCR1(n) |= (1 << 6);

    ICBRL(n) = br;
    ICBRH(n) = br;

    ICMR1(n) = 0x00;
    ICMR2(n) = 0x00;
    ICMR3(n) = 0x00;

    ICFER(n) &= ~(1 << 0);

    ICCR1(n) &= ~(1 << 6);
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
