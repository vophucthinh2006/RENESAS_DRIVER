/**
 * @file    main.c
 * @brief   RTOS Kernel Demo — Three LED tasks on EK-RA6M5
 *
 * Demonstrates:
 *   - Three tasks at different priorities toggling P006, P007, P008.
 *   - Priority-based preemption: higher-priority tasks preempt lower.
 *   - Round-Robin: tasks at the same priority share CPU time equally.
 *   - OS_Task_Delay() for tick-based blocking (non-busy-wait).
 *
 * Hardware:
 *   Board:  EK-RA6M5 (R7FA6M5BH3CFC, LQFP176)
 *   LEDs:   P006 (LED1), P007 (LED2), P008 (LED3)
 *   Clock:  ICLK = 200 MHz via PLL (configured by CLK_Init in startup.c)
 *
 * GPIO Control — Direct Register Access (no FSP/HAL):
 *   Port 0 base = 0x40080000
 *   PDR   (direction):   PORT_BASE + 0x02  → bit = 1 for output
 *   PODR  (output data):  PORT_BASE + 0x00  → bit = 1 for HIGH
 *   PFS   (pin function):  0x40080800 + 0x40*port + 0x04*pin
 *   PWPR  (write-protect): 0x40080800 + 0x503
 */

#include <stdint.h>
#include "kernel.h"

/* ======================================================================
 * GPIO Register Definitions — Port 0 (P006, P007, P008)
 *
 * RA6M5 Hardware Manual §19.2:
 *   PORT_BASE  = 0x4008_0000
 *   Port stride = 0x20 per port
 *   PCNTR1 (offset 0x00): bits[31:16] = PODR, bits[15:0] = PDR
 *   PCNTR3 (offset 0x08): bits[31:16] = PORR, bits[15:0] = POSR
 *     POSR (Port Output Set Register): writing 1 sets the pin HIGH
 *     PORR (Port Output Reset Register): writing 1 sets the pin LOW
 *
 *   PFS (Pin Function Select):
 *     Base = 0x4008_0800
 *     PmnPFS = base + 0x40*m + 0x04*n
 *     Bit 16 (PMR): 0 = GPIO mode, 1 = peripheral function
 *
 *   PWPR (Write-Protect Register):
 *     Address = 0x4008_0D03
 *     Unlock sequence:  PWPR = 0x00 (clear B0WI)
 *                       PWPR = 0x40 (set PFSWE)
 *     Lock sequence:    PWPR = 0x00 (clear PFSWE)
 *                       PWPR = 0x80 (set B0WI)
 * ====================================================================== */

#define PORT0_BASE      0x40080000UL
#define PORT0_PCNTR1    (*(volatile uint32_t *)(PORT0_BASE + 0x000U))
#define PORT0_PDR       (*(volatile uint16_t *)(PORT0_BASE + 0x002U))
#define PORT0_PCNTR3    (*(volatile uint32_t *)(PORT0_BASE + 0x008U))
#define PORT0_POSR      (*(volatile uint16_t *)(PORT0_BASE + 0x008U))  /* Set   */
#define PORT0_PORR      (*(volatile uint16_t *)(PORT0_BASE + 0x00AU))  /* Reset */

/* Pin Function Select registers for Port 0, pins 6/7/8 */
#define PFS_BASE        0x40080800UL
#define P006_PFS        (*(volatile uint32_t *)(PFS_BASE + 0x040U * 0U + 0x004U * 6U))
#define P007_PFS        (*(volatile uint32_t *)(PFS_BASE + 0x040U * 0U + 0x004U * 7U))
#define P008_PFS        (*(volatile uint32_t *)(PFS_BASE + 0x040U * 0U + 0x004U * 8U))

/* Write-Protect Register for PFS */
#define PWPR            (*(volatile uint8_t  *)(PFS_BASE + 0x503U))

/* PWPR bit positions */
#define PWPR_B0WI       (1U << 7)   /**< Bit 7: PFS Write Inhibit (B0WI)   */
#define PWPR_PFSWE      (1U << 6)   /**< Bit 6: PFS Write Enable  (PFSWE)  */

/* PFS bit positions */
#define PFS_PMR         (1U << 16)  /**< Bit 16: Port Mode — 0=GPIO, 1=peripheral */

/* Pin masks for P006, P007, P008 */
#define PIN6_MASK       (1U << 6)
#define PIN7_MASK       (1U << 7)
#define PIN8_MASK       (1U << 8)

/* ======================================================================
 * Static TCBs — no dynamic allocation
 * ====================================================================== */

static OS_TCB_t tcb_led1;      /* P006 — 200ms toggle, Priority 2 */
static OS_TCB_t tcb_led2;      /* P007 — 500ms toggle, Priority 2 */
static OS_TCB_t tcb_heartbeat; /* P008 —  50ms toggle, Priority 1 */

/* ======================================================================
 * GPIO Initialisation
 *
 * Sequence:
 *   1. Unlock PWPR (mandatory before writing any PFS register).
 *   2. Configure PFS for each pin: GPIO mode (PMR = 0), no pull-up.
 *   3. Lock PWPR.
 *   4. Set PDR bits for output direction.
 *   5. Clear PODR bits (LEDs off initially — active-low on EK-RA6M5).
 * ====================================================================== */

static void gpio_init(void)
{
    /* --- Step 1: Unlock PFS write protection ---
     *
     * PWPR security sequence (RA6M5 HW Manual §19.2.5):
     *   Write 0x00 → clears B0WI (bit 7), allows PFSWE to be modified.
     *   Write 0x40 → sets PFSWE (bit 6), enables PFS register writes.
     *
     * The two-step unlock prevents accidental PFS modification. */
    PWPR = 0x00U;
    PWPR = PWPR_PFSWE;

    /* --- Step 2: Configure pin functions ---
     *
     * PFS = 0x00000000:
     *   PMR  (bit 16) = 0 → GPIO mode (not peripheral function)
     *   ASEL (bit  7) = 0 → digital (not analog)
     *   NCODR(bit  6) = 0 → CMOS output (not open-drain)
     *   All other bits = 0 → no pull-up, no event trigger */
    P006_PFS = 0x00000000UL;
    P007_PFS = 0x00000000UL;
    P008_PFS = 0x00000000UL;

    /* --- Step 3: Lock PFS write protection ---
     *   Write 0x00 → clears PFSWE.
     *   Write 0x80 → sets B0WI, preventing further PFSWE changes. */
    PWPR = 0x00U;
    PWPR = PWPR_B0WI;

    /* --- Step 4: Set pin direction to OUTPUT ---
     *
     * PDR (Port Direction Register) — 16-bit register at PORT_BASE + 0x02.
     * Bit n = 1 → pin n is output.
     * We OR-in bits 6, 7, 8 to preserve other pin configurations. */
    PORT0_PDR |= (uint16_t)(PIN6_MASK | PIN7_MASK | PIN8_MASK);

    /* --- Step 5: LEDs OFF initially ---
     *
     * EK-RA6M5 LEDs are active-low: HIGH = OFF, LOW = ON.
     * Use POSR (Port Output Set Register) to set pins HIGH.
     * Writing 1 to a bit in POSR sets the corresponding PODR bit. */
    PORT0_POSR = (uint16_t)(PIN6_MASK | PIN7_MASK | PIN8_MASK);
}

/**
 * @brief  Toggle a pin on Port 0 using atomic set/reset registers.
 *
 * @param  pin_mask  Bitmask for the pin (e.g. PIN6_MASK).
 *
 * Reads the current output state from PCNTR1[31:16] (PODR field).
 * If the pin is HIGH, writes PORR to clear it.
 * If the pin is LOW, writes POSR to set it.
 *
 * Using POSR/PORR is race-free (no read-modify-write on PODR needed).
 */
static void gpio_toggle(uint16_t pin_mask)
{
    /* Read current PODR from the upper 16 bits of PCNTR1. */
    uint32_t pcntr1 = PORT0_PCNTR1;
    uint16_t podr   = (uint16_t)(pcntr1 >> 16);

    if ((podr & pin_mask) != 0U) {
        /* Pin is HIGH → reset (set LOW).
         * PORR: writing 1 clears the corresponding PODR bit. */
        PORT0_PORR = pin_mask;
    } else {
        /* Pin is LOW → set (set HIGH).
         * POSR: writing 1 sets the corresponding PODR bit. */
        PORT0_POSR = pin_mask;
    }
}

/* ======================================================================
 * Task Functions
 *
 * Each task runs in an infinite loop, toggling its assigned LED and
 * then yielding the CPU via OS_Task_Delay().  The delay puts the task
 * into BLOCKED state; the SysTick handler moves it back to READY
 * when the delay expires.
 * ====================================================================== */

/**
 * @brief  Task 1: Toggle LED1 (P006) every 200 ms.
 *         Priority 2 — same as LED2, so they share CPU via Round-Robin.
 */
static void task_led1(void *arg)
{
    (void)arg;
    for (;;) {
        gpio_toggle(PIN6_MASK);
        OS_Task_Delay(200U);
    }
}

/**
 * @brief  Task 2: Toggle LED2 (P007) every 500 ms.
 *         Priority 2 — same as LED1, Round-Robin scheduling applies.
 */
static void task_led2(void *arg)
{
    (void)arg;
    for (;;) {
        gpio_toggle(PIN7_MASK);
        OS_Task_Delay(500U);
    }
}

/**
 * @brief  Task 3: Heartbeat on LED3 (P008) every 50 ms.
 *         Priority 1 — HIGHER than LED1/LED2, will preempt them.
 */
static void task_heartbeat(void *arg)
{
    (void)arg;
    for (;;) {
        gpio_toggle(PIN8_MASK);
        OS_Task_Delay(50U);
    }
}

/* ======================================================================
 * Application Entry Point
 * ====================================================================== */

int main(void)
{
    /* Initialise GPIO: P006, P007, P008 as push-pull outputs. */
    gpio_init();

    /* Initialise the RTOS kernel (creates idle task). */
    OS_Init();

    /* Create application tasks.
     * Priority 1 = higher (heartbeat preempts LED tasks).
     * Priority 2 = lower  (LED1 and LED2 round-robin at same level). */
    (void)OS_Task_Create(&tcb_led1,      task_led1,      (void *)0, 2U, "LED1_Fast");
    (void)OS_Task_Create(&tcb_led2,      task_led2,      (void *)0, 2U, "LED2_Slow");
    (void)OS_Task_Create(&tcb_heartbeat, task_heartbeat,  (void *)0, 1U, "Heartbeat");

    /* Start the kernel.  Configures SysTick (1 ms, 200 MHz ICLK),
     * sets PendSV/SysTick to lowest NVIC priority, and launches
     * the highest-priority ready task.  This function never returns. */
    OS_Start();

    /* Never reached. */
    return 0;
}
