/*
 * main.c  —  UART Driver Test Program for TESTING_2 bare-metal project.
 *
 * Tests UART functions from Driver/Include/SCI.h and Driver/Source/SCI.c.
 * Board: EK-RA6M5 (R7FA6M5BH3CFC, LQFP176)
 *
 * This example sends a repeating message on UART0 at 115200 baud.
 * Connect a USB-UART adapter to the UART0 pins or use the board UART header.
 */

#include <stdint.h>
#include "../Driver/Include/SCI.h"
#include "../Driver/Include/GPIO.h"

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
    GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_SET); /* LED OFF */
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

/* UART self-test: write to TDR and check if SSR shows ready */
static uint8_t uart_self_test(UART_t uart)
{
    volatile uint8_t *TDR;
    volatile uint8_t *SSR;

    switch (uart) {
        case UART7:
            TDR = &SCI7_TDR;
            SSR = &SCI7_SSR;
            break;
        default:
            return 0; /* fail */
    }

    /* Wait for TX buffer empty */
    uint32_t timeout = 10000;
    while (!(*SSR & SSR_TDRE) && timeout--) {
        delay_ms(1);
    }
    if (timeout == 0) return 0; /* timeout */

    /* Write test byte */
    *TDR = 'T';

    /* Clear TDRE flag */
    *SSR &= (uint8_t)(~(uint8_t)SSR_TDRE);

    /* Wait for TX buffer empty again */
    timeout = 10000;
    while (!(*SSR & SSR_TDRE) && timeout--) {
        delay_ms(1);
    }

    return (timeout > 0) ? 1 : 0; /* pass/fail */
}

int main(void)
{
    led_init();

    /* Test UART7 peripheral */
    UART_Init(UART7, 115200U);

    /* Self-test UART */
    uint8_t uart_ok = uart_self_test(UART7);

    if (uart_ok) {
        /* UART works - blink fast */
        UART_SendString(UART7, "UART7 self-test PASSED\r\n");
        UART_SendString(UART7, "Actual baud rate: ~125000 (set terminal to 125000 8N1)\r\n");
        UART_SendString(UART7, "Connect TX=P613, RX=P614\r\n");
        while (1) {
            UART_SendString(UART7, "RA6M5 UART7 alive\r\n");
            led_blink(1, 500);  /* Slower blink: 500ms on, 500ms off */
            delay_ms(500);
        }
    } else {
        /* UART failed - blink slow */
        while (1) {
            led_blink(2, 500);  /* 2 blinks: 500ms on, 500ms off each */
            delay_ms(1000);
        }
    }

    return 0;
}

