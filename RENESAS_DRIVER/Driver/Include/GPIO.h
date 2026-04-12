#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>

#define PORT_BASE      0x40080000UL
#define PORT_STRIDE    0x0020UL
#define PORT_ADDR(p)   (PORT_BASE + PORT_STRIDE * (p))

#define PORT_PCNTR1(p) (*(volatile uint32_t *)(uintptr_t)(PORT_ADDR(p) + 0x000))
#define PORT_PDR(p)    (*(volatile uint16_t *)(uintptr_t)(PORT_ADDR(p) + 0x002))
#define PORT_PCNTR2(p) (*(volatile uint32_t *)(uintptr_t)(PORT_ADDR(p) + 0x004))
#define PORT_PCNTR3(p) (*(volatile uint32_t *)(uintptr_t)(PORT_ADDR(p) + 0x008))
#define PORT_PCNTR4(p) (*(volatile uint32_t *)(uintptr_t)(PORT_ADDR(p) + 0x00C))

#define PFS_BASE       0x40080800UL
/* PMR bit[16]: 0=GPIO, 1=Peripheral function (R7FA6M5BH.h: R_PFS_PORT_PIN_PmnPFS_PMR_Pos=16) */
#define PmnPFS_PMR     (1U << 16)
#define PmnPFS_NCODR   (1U << 6)
#define PmnPFS_PCR     (1U << 4)
#define PmnPFS_PSEL(x) ((x) << 8)

#define PmnPFS(m, n)   (*(volatile uint32_t *)(uintptr_t)(PFS_BASE + 0x000 + 0x040*(m) + 0x004*(n)))
#define PWPR           (*(volatile uint8_t *)(uintptr_t)(PFS_BASE + 0x503))

typedef enum{
    GPIO_PORT0 = '0',
    GPIO_PORT1 = '1',
    GPIO_PORT2 = '2',
    GPIO_PORT3 = '3',
    GPIO_PORT4 = '4',
    GPIO_PORT5 = '5',
    GPIO_PORT6 = '6',
    GPIO_PORT7 = '7',
    GPIO_PORT8 = '8',
    GPIO_PORT9 = '9',
    GPIO_PORTA = 'A',
    GPIO_PORTB = 'B'
}GPIO_PORT_t;

//CNF
typedef enum{
    //INPUT
    GPIO_CNF_IN_FLT    = 0,   /* digital input, floating (no pull)              */
    GPIO_CNF_IN_PU     = 1,   /* digital input with internal pull-up            */
    GPIO_CNF_IN_PD     = 2,   /* pull-down (not on RA6M5; use external resistor)*/
    GPIO_CNF_IN_ANA    = 3,   /* analog mode (ASEL=1)                           */
    GPIO_CNF_IN_RES    = 4,   /* reserved                                       */
    //OUTPUT
    GPIO_CNF_OUT_PP    = 5,   /* push-pull output                               */
    GPIO_CNF_OUT_OD    = 6,   /* open-drain output (NCODR=1)                    */
    GPIO_CNF_AF_PP     = 7,   /* peripheral function, push-pull (PMR=1)         */
    GPIO_CNF_AF_OD     = 8    /* peripheral function, open-drain (PMR=1,NCODR=1)*/
}GPIO_CNF_t;

//MODE
typedef enum{
    GPIO_MODE_IN      = 0b00,
    GPIO_MODE_OUT_10M = 0b01,
    GPIO_MODE_OUT_2M  = 0b10,
    GPIO_MODE_OUT_50M = 0b11
}GPIO_MODE_t;

//PINSTATE
typedef enum{
    GPIO_PIN_RESET = 0b0,
    GPIO_PIN_SET   = 0b1
}GPIO_PINSTATE_t;

uint8_t GPIO_PortToIndex(GPIO_PORT_t port);
void GPIO_Config(GPIO_PORT_t port,
        uint8_t pin,
        GPIO_CNF_t cnf,
        GPIO_MODE_t mode
        );
void GPIO_Write_Pin(
        GPIO_PORT_t port,
        uint8_t pin,
        GPIO_PINSTATE_t state
        );
uint8_t GPIO_Read_Pin(
        GPIO_PORT_t port,
        uint8_t pin
        );

#endif
