#include "GPIO.h"

uint8_t GPIO_PortToIndex(GPIO_PORT_t port)
{
    if (port >= '0' && port <= '9')
        return port - '0';
    else if (port >= 'A' && port <= 'B')
        return port - 'A' + 10;
    else
        return 0;
}
void GPIO_Config(GPIO_PORT_t port,
        uint8_t pin,
        GPIO_CNF_t cnf,
        GPIO_MODE_t mode
        ){
    uint8_t p = GPIO_PortToIndex(port);

    PWPR = 0x00;
    PmnPFS(p, pin) = 0x00000000;

    if (mode == GPIO_MODE_IN) PORT_PDR(p) &= ~(1 << pin);
    else PORT_PDR(p) |= (1 << pin);

    switch (cnf){
        case GPIO_CNF_IN_FLT:
            break;
        case GPIO_CNF_IN_PU:
            PmnPFS(p, pin) |= (1 << 4)
            PORT_PCNTR3(p) = (1 << pin);
            break;
        case GPIO_CNF_IN_PD:
            PORT_PCNTR4(p) = (1 << pin);
            break;
        case GPIO_CNF_IN_ANA:
            break;
        case GPIO_CNF_OUT_PP:
            break;
        case GPIO_CNF_OUT_OD:
            PmnPFS(p, pin) |= (1 << 6);
            break;
        case GPIO_CNF_AF_PP:
        case GPIO_CNF_AF_OD:
            PmnPFS(p, pin) |= (1 << 0);
            break;

        default:
            break;
    }
    if (cnf != GPIO_CNF_AF_PP && cnf != GPIO_CNF_AF_OD){
        PmnPFS(p, pin) &= ~(1 << 0);
    }
    PWPR = 0x80;
}
void GPIO_Write_Pin(
        GPIO_PORT_t port,
        uint8_t pin,
        GPIO_PINSTATE_t state
        ){
    uint8_t p = GPIO_PortToIndex(port);

    if (state == GPIO_PIN_SET)
        PORT_PCNTR3(p) = (1 << pin);
    else
        PORT_PCNTR4(p) = (1 << pin);
}
uint8_t GPIO_Read_Pin(
        GPIO_PORT_t port,
        uint8_t pin
        ){
    uint8_t p = GPIO_PortToIndex(port);
    return (PORT_PCNTR2(p) >> pin) & 1;
}
