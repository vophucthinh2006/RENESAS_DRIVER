/**
 * @file    main.c
 * @brief   UART Baremetal Test — EK-RA6M5 (RA6M5 / Cortex-M33)
 *
 * Active: bare-metal UART transmit test via debug_print() with LED checkpoints.
 * Clock: 8 MHz MOCO, configured by CLK_Init() in startup.c.
 * UART:  SCI7 (TX=P613, RX=P614), 115200 baud (configured in rtos_config.h).
 *
 * LED debug checkpoints (active-HIGH: HIGH=ON, LOW=OFF):
 *   LED1 (P006, Blue)  — blinks 10× fast: TDRE=0 (UART stuck, reflash needed)
 *                        solid ON:         TDRE=1 (UART functional, check wiring)
 *   LED2 (P007, Green) — lights after first debug_print() returns
 *   LED3 (P008, Red)   — toggles every loop iteration (heartbeat)
 *
 * Diagnostic guide:
 *   LED1 blinks 10× fast → TDRE never set after init → module stop still active
 *                           → rebuild + reflash required
 *   LED1 solid, no output  → UART is transmitting on P613, physical wiring issue
 *                           → check USB-UART adapter: adapter-RX→P613, GND→GND
 *   LED3 blinking           → main loop alive
 */

#include "debug_print.h"
#include "drv_uart.h"
#include "GPIO.h"
#include "rtos_config.h"
#include <stdint.h>

/* ======================================================================
 * LED pin assignments — EK-RA6M5
 *   LED1 (Blue)  = P006,  LED2 (Green) = P007,  LED3 (Red) = P008
 * Active-HIGH: drive HIGH = ON, drive LOW = OFF.
 * ====================================================================== */
#define LED_PORT    GPIO_PORT0
#define LED1_PIN    6U
#define LED2_PIN    7U
#define LED3_PIN    8U

#define LED1_ON()   GPIO_Write_Pin(LED_PORT, LED1_PIN, GPIO_PIN_SET)
#define LED2_ON()   GPIO_Write_Pin(LED_PORT, LED2_PIN, GPIO_PIN_SET)
#define LED3_ON()   GPIO_Write_Pin(LED_PORT, LED3_PIN, GPIO_PIN_SET)
#define LED1_OFF()  GPIO_Write_Pin(LED_PORT, LED1_PIN, GPIO_PIN_RESET)
#define LED2_OFF()  GPIO_Write_Pin(LED_PORT, LED2_PIN, GPIO_PIN_RESET)
#define LED3_OFF()  GPIO_Write_Pin(LED_PORT, LED3_PIN, GPIO_PIN_RESET)

static inline void led3_toggle(void)
{
    uint8_t cur = GPIO_Read_Pin(LED_PORT, LED3_PIN);
    GPIO_Write_Pin(LED_PORT, LED3_PIN,
                   (cur != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/* ======================================================================
 * LED GPIO Initialisation — push-pull output, start OFF.
 * ====================================================================== */
static void led_init(void)
{
    GPIO_Config(LED_PORT, LED1_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
    GPIO_Config(LED_PORT, LED2_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
    GPIO_Config(LED_PORT, LED3_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);

    LED1_OFF();
    LED2_OFF();
    LED3_OFF();
}

/* ======================================================================
 * Busy-wait delay — calibrated for 8 MHz MOCO, -O0.
 * ~4000 iterations ≈ 1 ms.
 * ====================================================================== */
static void delay_ms_bm(uint32_t ms)
{
    volatile uint32_t n = ms * 4000U;
    while (n-- != 0U) { __asm volatile("nop"); }
}

/* ======================================================================
 * Application Entry Point
 * ====================================================================== */
int main(void)
{
    /* --- Checkpoint 0: LED init ---------------------------------------- */
    led_init();

    /* --- Checkpoint 1: UART init -------------------------------------- */
    debug_print_init();   /* UART_Init(SCI7, 115200) */

    /*
     * TDRE diagnostic: read SSR immediately after UART_Init.
     *
     *   TDRE = 1  →  SCI7 module stop cleared, shift register active.
     *                UART is transmitting on P613.
     *                LED1 turns solid ON.
     *
     *   TDRE = 0  →  SCI7 still in module stop (MSTPCRB bit24 not cleared).
     *                All UART_SendChar calls will time-out silently.
     *                Cause: old binary on MCU. Rebuild + reflash.
     *                LED1 blinks 10× fast then turns solid.
     */
    uint8_t tdre_ok = (SCI_SSR(OS_DEBUG_UART_CHANNEL) & SSR_TDRE) ? 1U : 0U;

    if (!tdre_ok)
    {
        /* Blink LED1 10× fast: UART stuck — module stop active */
        for (uint8_t i = 0U; i < 10U; i++)
        {
            LED1_ON();
            delay_ms_bm(80U);
            LED1_OFF();
            delay_ms_bm(80U);
        }
    }

    LED1_ON();   /* LED1 solid → UART_Init done (TDRE=1: OK, TDRE=0: stuck) */

    /* --- Checkpoint 2: first transmission ----------------------------- */
    debug_print("\r\n=== UART Baremetal Test ===\r\n");
    debug_print("Target : RA6M5 EK-RA6M5\r\n");
    debug_print("Clock  : 8 MHz MOCO\r\n");
    debug_print("Channel: UART%u @ %u baud\r\n",
                (unsigned)OS_DEBUG_UART_CHANNEL,
                (unsigned)OS_DEBUG_UART_BAUDRATE);
    debug_print("TX pin : P6%02u\r\n", (unsigned)(13U));
    debug_print("TDRE after init: %s\r\n", tdre_ok ? "OK (1)" : "FAIL (0) - check MSTPCRB");
    debug_print("===========================\r\n\r\n");
    LED2_ON();   /* LED2 solid → first debug_print() returned */

    /* --- Checkpoint 3: main loop heartbeat ---------------------------- */
    uint32_t tick = 0U;
    for (;;)
    {
        debug_print("[%u] Hello RA6M5 P613\r\n", (unsigned)tick);
        tick++;

        led3_toggle();
        delay_ms_bm(500U);
    }

    return 0;
}
