#ifndef IIC_H
#define IIC_H
#include <stdint.h>

#define RIIC_BASE     0x4009F000UL
#define RIIC_STRIDE   0x0100UL
#define RIIC_ADDR(n)  (RIIC_BASE + RIIC_STRIDE * (n))

#define ICCR1(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x00))
#define ICCR2(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x01))
#define ICMR1(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x02))
#define ICMR2(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x03))
#define ICMR3(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x04))
#define ICFER(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x05))
#define ICSER(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x06))
#define ICIER(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x07))
#define ICSR1(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x08))
#define ICSR2(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x09))
#define ICBRL(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x10))
#define ICBRH(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x11))
#define ICDRT(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x12))
#define ICDRR(n)      (*(volatile uint8_t *)(RIIC_ADDR(n) + 0x13))

#define IIC0WU_BASE   0x4009F014UL

#define ICWUR         (*(volatile uint8_t *)(IIC0WU_BASE + 0x00))
#define ICWUR2        (*(volatile uint8_t *)(IIC0WU_BASE + 0x01))

typedef enum{
    I2C0,
    I2C1,
    I2C2
}I2C_t;

typedef enum{
    I2C_PIN_A,
    I2C_PIN_B
}I2C_PINCFG_t;

//SPEED
typedef enum{
    I2C_SPEED_STANDARD = 100000,
    I2C_SPEED_FAST     = 400000
}I2C_SPEED_t;

//WRITE_READ
typedef enum{
    I2C_LSB_TRANSMIT = 0,
    I2C_LSB_RECEIVE  = 1
}I2C_LSB_t;

//ACK-RECIEVE
typedef enum{
    I2C_ACK_CONTINUE,
    I2C_ACK_STOP
}I2C_ACK_t;

void I2C_Clock_Init(I2C_t I2C_);
void I2C_PinConfig(I2C_t I2C_);
void I2C_Init(
    I2C_t I2C_,
    uint8_t clock_frequency_MHz,
    I2C_SPEED_t speed_kHz
);
void I2C_Start(I2C_t I2C_);
uint8_t I2C_Transmit_Address(I2C_t I2C_, uint8_t address, I2C_LSB_t write_read);
void I2C_Stop(I2C_t I2C_);
uint8_t I2C_Master_Transmit_Data(I2C_t I2C_, uint8_t *data, uint8_t data_length);
uint8_t I2C_Master_Receive_Data(I2C_t I2C_, uint8_t *data, uint8_t data_length);

#endif
