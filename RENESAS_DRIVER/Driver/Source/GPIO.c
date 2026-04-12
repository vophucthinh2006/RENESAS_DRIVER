#include "GPIO.h"

uint8_t GPIO_PortToIndex(GPIO_PORT_t port)
{
    if (port >= '0' && port <= '9')
        return (uint8_t)(port - '0');
    else if (port >= 'A' && port <= 'B')
        return (uint8_t)(port - 'A' + 10);
    else
        return 0U;
}

/*
 * GPIO_Config - Configure a pin direction and function.
 *
 * PWPR unlock sequence (RA6M5 HW manual, section 19.2.5):
 *   1. PWPR = 0x00  clear B0WI  (allows changing PFSWE)
 *   2. PWPR = 0x40  set  PFSWE  (allows writing PFS registers)
 *      ... write PFS ...
 *   3. PWPR = 0x00  clear PFSWE
 *   4. PWPR = 0x80  set  B0WI   (locks PFS writes)
 *
 * PMR bit is bit[16] of PmnPFS (IOPORT_PRV_PERIPHERAL_FUNCTION = 1U<<16)
 * RA6M5 has no built-in pull-down; GPIO_CNF_IN_PD requires an external resistor.
 */
void GPIO_Config(GPIO_PORT_t port,
                 uint8_t     pin,
                 GPIO_CNF_t  cnf,
                 GPIO_MODE_t mode)
{
    uint8_t p = GPIO_PortToIndex(port);

    /* --- Unlock PFS --- */
    PWPR = 0x00U;          /* Step 1: clear B0WI  */
    PWPR = 0x40U;          /* Step 2: set  PFSWE  */

    /* Reset PFS for this pin (GPIO, no analog, no IRQ, no pull) */
    PmnPFS(p, pin) = 0x00000000U;

    switch (cnf)
    {
        case GPIO_CNF_IN_PU:
            PmnPFS(p, pin) |= (1U << 4);              /* PCR = 1, internal pull-up   */
            break;
        case GPIO_CNF_OUT_OD:
            PmnPFS(p, pin) |= (1U << 6);              /* NCODR = 1, N-ch open-drain  */
            break;
        case GPIO_CNF_AF_PP:
            PmnPFS(p, pin) |= (1U << 16);             /* PMR = 1, peripheral func    */
            break;
        case GPIO_CNF_AF_OD:
            PmnPFS(p, pin) |= (1U << 16) | (1U << 6); /* PMR=1, NCODR=1              */
            break;
        case GPIO_CNF_IN_ANA:
            PmnPFS(p, pin) |= (1U << 15);             /* ASEL = 1, analog mode       */
            break;
        case GPIO_CNF_IN_FLT:  /* digital input, no pull -- already cleared */
        case GPIO_CNF_OUT_PP:  /* push-pull output       -- already cleared */
        case GPIO_CNF_IN_PD:   /* pull-down: not avail on RA6M5 (use external resistor) */
        default:
            break;
    }

    /* --- Lock PFS --- */
    PWPR = 0x00U;          /* Step 3: clear PFSWE */
    PWPR = 0x80U;          /* Step 4: set  B0WI   */

    /* --- Set direction in PCNTR1.PDR --- */
    if (mode == GPIO_MODE_IN)
        PORT_PDR(p) &= (uint16_t)(~(uint16_t)(1U << pin));
    else
        PORT_PDR(p) |=  (uint16_t)(1U << pin);
}
/*
 * GPIO_Write_Pin - Atomically set or clear a GPIO output.
 *
 * PCNTR3 layout (RA6M5 HW manual §19.2.2):
 *   bits [15: 0] = POSR  (Port Output Set   Register) — write 1 to drive pin HIGH
 *   bits [31:16] = PORR  (Port Output Reset Register) — write 1 to drive pin LOW
 *
 * PCNTR4 is the ELC Event Output register; it is NOT used here.
 */
void GPIO_Write_Pin(GPIO_PORT_t     port,
                    uint8_t         pin,
                    GPIO_PINSTATE_t state)
{
    uint8_t p = GPIO_PortToIndex(port);

    if (state == GPIO_PIN_SET)
        PORT_PCNTR3(p) = (uint32_t)(1U << pin);          /* POSR: drive HIGH */
    else
        PORT_PCNTR3(p) = (uint32_t)(1U << pin) << 16;    /* PORR: drive LOW  */
}
uint8_t GPIO_Read_Pin(GPIO_PORT_t port,
                      uint8_t     pin)
{
    uint8_t p = GPIO_PortToIndex(port);
    /* PCNTR2 bits [15:0] = PIDR (Port Input Data Register) */
    return (uint8_t)((PORT_PCNTR2(p) >> pin) & 1U);
}
