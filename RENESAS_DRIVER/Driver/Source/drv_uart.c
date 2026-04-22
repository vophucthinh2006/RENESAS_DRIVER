#include "drv_uart.h"
#include "drv_clk.h"
#include "GPIO.h"

/* -----------------------------------------------------------------------
 * uart_clock_init — release module stop for one SCI channel.
 * Internal helper; callers use UART_Init().
 * ----------------------------------------------------------------------- */
static void uart_clock_init(uint8_t n)
{
    CLK_ModuleStart_SCI((SCI_t)n);
}

/* -----------------------------------------------------------------------
 * UART_PinConfig — map UART channel to TX/RX port-pin pairs.
 *
 * Pin mapping (EK-RA6M5 schematic + RA6M5 pin function table):
 *   UART0 : RX=P110, TX=P111  PSEL=0x04
 *   UART1 : RX=P708, TX=P709  PSEL=0x05
 *   UART2 : RX=P301, TX=P302  PSEL=0x04
 *   UART3 : RX=P309, TX=P310  PSEL=0x05
 *   UART4 : RX=P206, TX=P205  PSEL=0x04
 *   UART5 : RX=P502, TX=P501  PSEL=0x05
 *   UART6 : RX=P505, TX=P506  PSEL=0x04
 *   UART7 : RX=P614, TX=P613  PSEL=0x05  ← board UART
 *   UART8 : RX=P607, TX=PA00  PSEL=0x04
 *   UART9 : RX=P110, TX=P109  PSEL=0x05
 * ----------------------------------------------------------------------- */
static void uart_pin_config(UART_t uart)
{
    uint8_t  tx_port, tx_pin;
    uint8_t  rx_port, rx_pin;
    uint32_t psel;

    switch (uart)
    {
        case UART0: tx_port=1;  tx_pin=1;  rx_port=1;  rx_pin=0;  psel=0x04U; break;
        case UART1: tx_port=7;  tx_pin=9;  rx_port=7;  rx_pin=8;  psel=0x05U; break;
        case UART2: tx_port=3;  tx_pin=2;  rx_port=3;  rx_pin=1;  psel=0x04U; break;
        case UART3: tx_port=3;  tx_pin=10; rx_port=3;  rx_pin=9;  psel=0x05U; break;
        case UART4: tx_port=2;  tx_pin=5;  rx_port=2;  rx_pin=6;  psel=0x04U; break;
        case UART5: tx_port=5;  tx_pin=1;  rx_port=5;  rx_pin=2;  psel=0x05U; break;
        case UART6: tx_port=5;  tx_pin=6;  rx_port=5;  rx_pin=5;  psel=0x04U; break;
        case UART7: tx_port=6;  tx_pin=13; rx_port=6;  rx_pin=14; psel=0x05U; break;
        case UART8: tx_port=10; tx_pin=0;  rx_port=6;  rx_pin=7;  psel=0x04U; break;
        case UART9: tx_port=1;  tx_pin=9;  rx_port=1;  rx_pin=10; psel=0x05U; break;
        default: return;
    }

    PWPR = 0x00U;   /* Step 1: clear B0WI  (allows changing PFSWE) */
    PWPR = 0x40U;   /* Step 2: set  PFSWE  (allows writing PFS)     */

    /* RX pin: peripheral function, input direction */
    PmnPFS(rx_port, rx_pin)  = PmnPFS_PSEL(psel);
    PmnPFS(rx_port, rx_pin) |= PmnPFS_PMR;
    PORT_PDR(rx_port) &= (uint16_t)(~(uint16_t)(1U << rx_pin));

    /* TX pin: peripheral function, output direction */
    PmnPFS(tx_port, tx_pin)  = PmnPFS_PSEL(psel);
    PmnPFS(tx_port, tx_pin) |= PmnPFS_PMR;
    PORT_PDR(tx_port) |= (uint16_t)(1U << tx_pin);

    PWPR = 0x00U;   /* Step 3: clear PFSWE */
    PWPR = 0x80U;   /* Step 4: set  B0WI   (locks PFS writes) */
}

/* -----------------------------------------------------------------------
 * UART_Init — initialise one SCI channel in async UART mode.
 *
 * BRR formula with SEMR: BGDM=1 (bit6), ABCS=1 (bit4):
 *   BRR = PCLKB / (4 × baudrate) − 1
 *
 * Example at PCLKB=8 MHz, 115200 baud:
 *   BRR = 8 000 000 / (4 × 115 200) − 1 = 16  → actual 117 647 baud (2.1% error)
 * ----------------------------------------------------------------------- */
void UART_Init(UART_t uart, uint32_t baudrate)
{
    uint8_t n = (uint8_t)uart;
    if (n > 9U) { return; }

    uart_clock_init(n);
    uart_pin_config(uart);

    SCI_SCR(n)  = 0x00U;                                        /* disable TX/RX while configuring */
    SCI_SMR(n)  = 0x00U;                                        /* async, 8-bit, no parity, 1 stop */
    SCI_SEMR(n) = (uint8_t)(SEMR_BGDM | SEMR_ABCS);            /* BGDM=1(b6), ABCS=1(b4) → /4    */
    SCI_BRR(n)  = (uint8_t)((PCLKB / (4UL * baudrate)) - 1U);  /* baud rate register              */
    SCI_SCR(n)  = (uint8_t)(SCR_TE | SCR_RE);                   /* enable TX and RX                */
}

/* -----------------------------------------------------------------------
 * UART_SendChar — blocking transmit of one byte.
 * Hardware automatically clears TDRE when TDR is written; no manual clear.
 * ----------------------------------------------------------------------- */
void UART_SendChar(UART_t uart, char data)
{
    uint8_t n = (uint8_t)uart;
    if (n > 9U) { return; }

    while (!(SCI_SSR(n) & SSR_TDRE)) {}   /* wait: TX buffer empty */
    SCI_TDR(n) = (uint8_t)data;           /* write byte            */
}

/* -----------------------------------------------------------------------
 * UART_SendString — transmit a null-terminated string.
 * ----------------------------------------------------------------------- */
void UART_SendString(UART_t uart, const char *str)
{
    while (*str != '\0')
    {
        UART_SendChar(uart, *str++);
    }
}

/* -----------------------------------------------------------------------
 * UART_ReceiveChar — blocking receive of one byte.
 * ----------------------------------------------------------------------- */
char UART_ReceiveChar(UART_t uart)
{
    uint8_t n = (uint8_t)uart;
    if (n > 9U) { return 0; }

    while (!(SCI_SSR(n) & SSR_RDRF)) {}   /* wait: RX data ready */
    return (char)SCI_RDR(n);
}
