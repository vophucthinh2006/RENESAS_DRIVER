/**
 * @file    main.c
 * @brief   RTOS Demo — Tasks + Semaphore + Software Timer on EK-RA6M5
 *
 * Demonstrates:
 *   Task 1: Toggles LED1 (P006) every 500 ms using OS_Task_Delay.
 *   Task 2: Waits on a semaphore posted by a Software Timer → toggles LED2
 * (P007). Task 3: High-priority preemptive task toggling LED3 (P008) every 100
 * ms.
 *
 * GPIO: Direct register access. No FSP/HAL.
 * Clock: ICLK = 200 MHz (PLL), configured by CLK_Init() in startup.c.
 */

#include "kernel.h"
#include "semaphore.h"
#include "software_timer.h"
#include <stdint.h>

/* ======================================================================
 * GPIO Register Definitions — Port 0 (P006, P007, P008)
 *
 * PORT_BASE = 0x4008_0000, stride = 0x20.
 * PCNTR1 (0x00): [31:16]=PODR (output data), [15:0]=PDR (direction).
 * POSR   (0x08): write 1 to set pin HIGH (atomic, no RMW).
 * PORR   (0x0A): write 1 to set pin LOW  (atomic, no RMW).
 *
 * PFS_BASE = 0x4008_0800.  PmnPFS = base + 0x40*port + 0x04*pin.
 * PWPR    = PFS_BASE + 0x503 (write-protect for PFS).
 * ====================================================================== */

#define PORT0_BASE 0x40080000UL
#define PORT0_PCNTR1 (*(volatile uint32_t *)(PORT0_BASE + 0x000U))
#define PORT0_PDR (*(volatile uint16_t *)(PORT0_BASE + 0x002U))
#define PORT0_POSR (*(volatile uint16_t *)(PORT0_BASE + 0x008U))
#define PORT0_PORR (*(volatile uint16_t *)(PORT0_BASE + 0x00AU))

#define PFS_BASE 0x40080800UL
#define P006_PFS (*(volatile uint32_t *)(PFS_BASE + 0x018U))
#define P007_PFS (*(volatile uint32_t *)(PFS_BASE + 0x01CU))
#define P008_PFS (*(volatile uint32_t *)(PFS_BASE + 0x020U))
#define PWPR (*(volatile uint8_t *)(PFS_BASE + 0x503U))

#define PWPR_B0WI (1U << 7)
#define PWPR_PFSWE (1U << 6)

#define PIN6_MASK (1U << 6)
#define PIN7_MASK (1U << 7)
#define PIN8_MASK (1U << 8)

/* ======================================================================
 * Static OS Objects — No malloc
 * ====================================================================== */

static OS_TCB_t tcb_task1; /* LED1 delay-based toggle   */
static OS_TCB_t tcb_task2; /* LED2 semaphore-driven     */
static OS_TCB_t tcb_task3; /* LED3 high-prio preemption */

static Semaphore_t sem_led2; /* Binary sem for Task 2     */
static Timer_t timer_led2;   /* Posts sem_led2 every 1 s  */

/* ======================================================================
 * GPIO Init — PWPR unlock, PFS config, direction, initial state
 * ====================================================================== */

static void gpio_init(void) {
  /* Unlock PFS write protection (RA6M5 HW Manual §19.2.5):
   * Step 1: clear B0WI → allows PFSWE modification.
   * Step 2: set PFSWE → enables PFS writes. */
  PWPR = 0x00U;
  PWPR = PWPR_PFSWE;

  /* Configure pins as GPIO (PMR=0, no pull-up, CMOS output). */
  P006_PFS = 0x00000000UL;
  P007_PFS = 0x00000000UL;
  P008_PFS = 0x00000000UL;

  /* Re-lock PFS. */
  PWPR = 0x00U;
  PWPR = PWPR_B0WI;

  /* Set direction: output. */
  PORT0_PDR |= (uint16_t)(PIN6_MASK | PIN7_MASK | PIN8_MASK);

  /* LEDs OFF initially (active-low: HIGH = off). */
  PORT0_POSR = (uint16_t)(PIN6_MASK | PIN7_MASK | PIN8_MASK);
}

/** Toggle a Port 0 pin using atomic POSR/PORR (no read-modify-write). */
static void gpio_toggle(uint16_t pin_mask) {
  uint16_t podr = (uint16_t)(PORT0_PCNTR1 >> 16);
  if ((podr & pin_mask) != 0U) {
    PORT0_PORR = pin_mask; /* HIGH → LOW  */
  } else {
    PORT0_POSR = pin_mask; /* LOW  → HIGH */
  }
}

/* ======================================================================
 * Task Functions
 * ====================================================================== */

/**
 * Task 1: Toggle LED1 (P006) every 500 ms using OS_Task_Delay.
 * Priority 3 — lower than Task 3 (preemptable).
 */
static void task_led1_delay(void *arg) {
  (void)arg;
  for (;;) {
    gpio_toggle(PIN6_MASK);
    OS_Task_Delay(500U);
  }
}

/**
 * Task 2: Wait for semaphore, then toggle LED2 (P007).
 * Priority 3 — same as Task 1 (Round-Robin).
 * The semaphore is posted by a software timer every 1000 ms.
 * This demonstrates Timer → Semaphore → Task synchronisation.
 */
static void task_led2_sem(void *arg) {
  (void)arg;
  for (;;) {
    /* Block until the software timer posts the semaphore. */
    int32_t result = OS_SemPend(&sem_led2, OS_WAIT_FOREVER);
    if (result == OS_OK) {
      gpio_toggle(PIN7_MASK);
    }
  }
}

/**
 * Task 3: High-priority heartbeat on LED3 (P008) — 100 ms toggle.
 * Priority 2 — HIGHER than Tasks 1 & 2, demonstrates preemption.
 */
static void task_led3_preempt(void *arg) {
  (void)arg;
  for (;;) {
    gpio_toggle(PIN8_MASK);
    OS_Task_Delay(100U);
  }
}

/* ======================================================================
 * Software Timer Callback
 *
 * Executed in Timer Daemon Task context (priority 1), NOT in ISR.
 * Posts the binary semaphore to wake Task 2.
 * ====================================================================== */

static void timer_led2_callback(void *arg) {
  Semaphore_t *sem = (Semaphore_t *)arg;
  (void)OS_SemPost(sem);
}

/* ======================================================================
 * Application Entry Point
 * ====================================================================== */

int main(void) {
  gpio_init();
  OS_Init();

  /* Create binary semaphore (initial=0 → Task 2 blocks immediately). */
  (void)OS_SemCreate(&sem_led2, 0U, 1U);

  /* Create software timer: auto-reload, 1000 ms period.
   * Callback posts sem_led2 → wakes Task 2. */
  (void)OS_TimerCreate(&timer_led2, timer_led2_callback, (void *)&sem_led2,
                       1000U, OS_TIMER_AUTO_RELOAD);
  (void)OS_TimerStart(&timer_led2);

  /* Create tasks.
   * Prio 2 = LED3 (highest user task — preempts others).
   * Prio 3 = LED1 and LED2 (same priority — Round-Robin). */
  (void)OS_Task_Create(&tcb_task1, task_led1_delay, (void *)0, 3U,
                       "LED1_Delay");
  (void)OS_Task_Create(&tcb_task2, task_led2_sem, (void *)0, 3U, "LED2_Sem");
  (void)OS_Task_Create(&tcb_task3, task_led3_preempt, (void *)0, 2U,
                       "LED3_Preempt");

  /* Start kernel — never returns.
   * Timer daemon (prio 1) and idle (prio 31) created by OS_Init(). */
  OS_Start();

  return 0;
}
