/**
 * @file    main.c
 * @brief   UART Baremetal Test — EK-RA6M5 (RA6M5 / Cortex-M33)
 *
 * Active: bare-metal UART transmit/receive test via debug_print().
 * Clock: 8 MHz MOCO, configured by CLK_Init() in startup.c.
 * UART:  SCI7 (TX=P613, RX=P614), 115200 baud (configured in rtos_config.h).
 *
 * RTOS demo (Tasks + Semaphore + Software Timer) is preserved in comments
 * at the bottom of this file for future reference.
 */

#include "debug_print.h"
#include "drv_uart.h"
#include <stdint.h>

/* ======================================================================
 * Busy-wait delay — calibrated for 8 MHz MOCO, -O0.
 * ~4000 iterations ≈ 1 ms.  Re-calibrate if optimisation level changes.
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
    /* Initialise debug UART (SCI7, 115200 baud). */
    debug_print_init();

    debug_print("\r\n=== UART Baremetal Test ===\r\n");
    debug_print("Target : RA6M5 EK-RA6M5\r\n");
    debug_print("Clock  : 8 MHz MOCO\r\n");
    debug_print("Channel: UART%u @ %u baud\r\n",
                (unsigned)OS_DEBUG_UART_CHANNEL,
                (unsigned)OS_DEBUG_UART_BAUDRATE);
    debug_print("===========================\r\n\r\n");

    uint32_t tick = 0U;

    for (;;)
    {
        debug_print("[%u] Hello from RA6M5\r\n", (unsigned)tick);
        tick++;
        delay_ms_bm(1000U);
    }

    return 0;
}

/* ======================================================================
 * RTOS Demo — disabled, preserved for reference
 *
 * Demonstrates: 3-task preemptive scheduler, counting semaphore,
 * software timer → semaphore → task synchronisation.
 *
 * To re-enable: remove this block comment, comment out the baremetal
 * section above, and restore the RTOS includes at the top of the file.
 *
 * -----------------------------------------------------------------------
 *
 * #include "kernel.h"
 * #include "semaphore.h"
 * #include "software_timer.h"
 *
 * // --- Port 0 GPIO (LED1=P006, LED2=P007, LED3=P008) -------------------
 *
 * #define PORT0_BASE   0x40080000UL
 * #define PORT0_PCNTR1 (*(volatile uint32_t *)(PORT0_BASE + 0x000U))
 * #define PORT0_PDR    (*(volatile uint16_t *)(PORT0_BASE + 0x002U))
 * #define PORT0_POSR   (*(volatile uint16_t *)(PORT0_BASE + 0x008U))
 * #define PORT0_PORR   (*(volatile uint16_t *)(PORT0_BASE + 0x00AU))
 *
 * #define PFS_BASE  0x40080800UL
 * #define P006_PFS  (*(volatile uint32_t *)(PFS_BASE + 0x018U))
 * #define P007_PFS  (*(volatile uint32_t *)(PFS_BASE + 0x01CU))
 * #define P008_PFS  (*(volatile uint32_t *)(PFS_BASE + 0x020U))
 * #define PWPR      (*(volatile uint8_t  *)(PFS_BASE + 0x503U))
 *
 * #define PWPR_B0WI   (1U << 7)
 * #define PWPR_PFSWE  (1U << 6)
 * #define PIN6_MASK   (1U << 6)
 * #define PIN7_MASK   (1U << 7)
 * #define PIN8_MASK   (1U << 8)
 *
 * static OS_TCB_t    tcb_task1;
 * static OS_TCB_t    tcb_task2;
 * static OS_TCB_t    tcb_task3;
 * static Semaphore_t sem_led2;
 * static Timer_t     timer_led2;
 *
 * static void gpio_init(void) {
 *     PWPR = 0x00U; PWPR = PWPR_PFSWE;
 *     P006_PFS = 0x00000000UL;
 *     P007_PFS = 0x00000000UL;
 *     P008_PFS = 0x00000000UL;
 *     PWPR = 0x00U; PWPR = PWPR_B0WI;
 *     PORT0_PDR |= (uint16_t)(PIN6_MASK | PIN7_MASK | PIN8_MASK);
 *     PORT0_POSR = (uint16_t)(PIN6_MASK | PIN7_MASK | PIN8_MASK);
 * }
 *
 * static void gpio_toggle(uint16_t pin_mask) {
 *     uint16_t podr = (uint16_t)(PORT0_PCNTR1 >> 16);
 *     if ((podr & pin_mask) != 0U) PORT0_PORR = pin_mask;
 *     else                         PORT0_POSR = pin_mask;
 * }
 *
 * // Task 1: LED1 (P006) toggle every 500 ms — priority 3
 * static void task_led1_delay(void *arg) {
 *     (void)arg;
 *     for (;;) { gpio_toggle(PIN6_MASK); OS_Task_Delay(500U); }
 * }
 *
 * // Task 2: LED2 (P007) driven by semaphore from software timer — priority 3
 * static void task_led2_sem(void *arg) {
 *     (void)arg;
 *     for (;;) {
 *         int32_t r = OS_SemPend(&sem_led2, OS_WAIT_FOREVER);
 *         if (r == OS_OK) gpio_toggle(PIN7_MASK);
 *     }
 * }
 *
 * // Task 3: LED3 (P008) high-priority heartbeat 100 ms — priority 2
 * static void task_led3_preempt(void *arg) {
 *     (void)arg;
 *     for (;;) { gpio_toggle(PIN8_MASK); OS_Task_Delay(100U); }
 * }
 *
 * // Timer callback: posts semaphore every 1000 ms
 * static void timer_led2_callback(void *arg) {
 *     (void)OS_SemPost((Semaphore_t *)arg);
 * }
 *
 * int main(void) {
 *     gpio_init();
 *     OS_Init();
 *
 *     (void)OS_SemCreate(&sem_led2, 0U, 1U);
 *     (void)OS_TimerCreate(&timer_led2, timer_led2_callback,
 *                          (void *)&sem_led2, 1000U, OS_TIMER_AUTO_RELOAD);
 *     (void)OS_TimerStart(&timer_led2);
 *
 *     (void)OS_Task_Create(&tcb_task1, task_led1_delay,   (void *)0, 3U, "LED1_Delay");
 *     (void)OS_Task_Create(&tcb_task2, task_led2_sem,     (void *)0, 3U, "LED2_Sem");
 *     (void)OS_Task_Create(&tcb_task3, task_led3_preempt, (void *)0, 2U, "LED3_Preempt");
 *
 *     OS_Start();  // never returns
 *     return 0;
 * }
 *
 * ====================================================================== */
