/*
 * main.c  —  UART Driver Test Program for TESTING_2 bare-metal project.
 *
 * Tests UART functions from Driver/Include/SCI.h and Driver/Source/SCI.c.
 * Board: EK-RA6M5 (R7FA6M5BH3CFC, LQFP176)
 *
 * UART7: TX=P613, RX=P614, 115200 baud (actual ~117647, 2.1% error — within spec)
 */

#include <stdint.h>
#include "SCI.h"
#include "GPIO.h"

#define LED1_PORT   GPIO_PORT0
#define LED1_PIN    6U

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 4000U;   /* ~8 MHz / 2 iterations per loop */
    while (count-- != 0U)
    {
        __asm volatile ("nop");
    }
}

static void led_init(void)
{
    GPIO_Config(LED1_PORT, LED1_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUT_10M);
    GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_SET); /* LED OFF (active-low) */
}

static void led_blink(uint32_t count, uint32_t period_ms)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
        delay_ms(period_ms);
        GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
        delay_ms(period_ms);
    }
}

/*
 * uart_self_test — verify UART7 TX path is functional.
 * Uses a timeout loop so the system cannot hang indefinitely.
 * Returns 1 = pass, 0 = fail/timeout.
 */
static uint8_t uart_self_test(UART_t uart)
{
    uint8_t n = (uint8_t)uart;

    /* Wait for TX buffer empty with timeout */
    uint32_t timeout = 10000U;
    while (!(SCI_SSR(n) & SSR_TDRE) && (timeout > 0U))
    {
        delay_ms(1U);
        timeout--;
    }
    if (timeout == 0U) { return 0U; }

    /* Write test byte — hardware clears TDRE automatically on TDR write */
    SCI_TDR(n) = (uint8_t)'T';

    /* Wait for TX to complete */
    timeout = 10000U;
    while (!(SCI_SSR(n) & SSR_TDRE) && (timeout > 0U))
    {
        delay_ms(1U);
        timeout--;
    }

    return (timeout > 0U) ? 1U : 0U;
}

int main(void)
{
    led_init();

    UART_Init(UART7, 115200U);

    uint8_t uart_ok = uart_self_test(UART7);

    if (uart_ok)
    {
        UART_SendString(UART7, "UART7 self-test PASSED\r\n");
        UART_SendString(UART7, "Baud: 115200 requested, ~117647 actual (2.1% error)\r\n");
        UART_SendString(UART7, "Connect TX=P613, RX=P614\r\n");
        while (1)
        {
            UART_SendString(UART7, "RA6M5 UART7 alive\r\n");
            led_blink(1U, 500U);
            delay_ms(500U);
        }
    }
    else
    {
        while (1)
        {
            led_blink(2U, 500U);
            delay_ms(1000U);
        }
    }

    return 0;
}
