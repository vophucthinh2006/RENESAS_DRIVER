/**
 * @file    kernel.c
 * @brief   Preemptive RTOS Kernel — Core Implementation
 *
 * Implements:
 *   - O(1) priority-bitmap scheduler using ARM __CLZ intrinsic.
 *   - Per-priority circular linked lists for Round-Robin rotation.
 *   - Static task creation with pre-built exception-return stack frames.
 *   - Tick-based blocking (OS_Task_Delay) with SysTick heartbeat.
 *   - Idle task executing WFI in a loop.
 *
 * Target: Renesas RA6M5 — R7FA6M5BH3CFC (ARM Cortex-M33, FPv5-SP)
 * Clock:  ICLK = 200 MHz (PLL).  SysTick reload = 199 999 → 1 ms tick.
 *
 * Zero FSP/HAL dependency.  Direct SCS register access only.
 */

#include "kernel.h"
#include <string.h>   /* memset */

/* ======================================================================
 * ARM System Control Space (SCS) Register Definitions
 *
 * Reference: ARMv8-M Architecture Reference Manual, §B3.3
 * All addresses are in the Private Peripheral Bus (PPB) region.
 * ====================================================================== */

/* --- SysTick Timer (SCS offset 0x010 – 0x01C) --- */

/** SysTick Control and Status Register.
 *  Bit 0  ENABLE:    1 = counter enabled.
 *  Bit 1  TICKINT:   1 = generate exception on count-to-zero.
 *  Bit 2  CLKSOURCE: 1 = processor clock (ICLK).  0 = external ref.
 *  Bit 16 COUNTFLAG: 1 = counter has counted to zero since last read. */
#define SYST_CSR    (*(volatile uint32_t *)0xE000E010UL)

/** SysTick Reload Value Register.
 *  Bits [23:0]: Value loaded into CVR when the counter reaches zero.
 *  The timer period = (RVR + 1) × clock_period.
 *  For 1 ms at 200 MHz: RVR = 200 000 − 1 = 199 999. */
#define SYST_RVR    (*(volatile uint32_t *)0xE000E014UL)

/** SysTick Current Value Register.
 *  Read:  current counter value.
 *  Write: any write clears the counter AND the COUNTFLAG bit. */
#define SYST_CVR    (*(volatile uint32_t *)0xE000E018UL)

/* SysTick CSR bit masks */
#define SYST_CSR_ENABLE     (1UL << 0)   /**< Counter enable.            */
#define SYST_CSR_TICKINT    (1UL << 1)   /**< Interrupt on zero-count.   */
#define SYST_CSR_CLKSOURCE  (1UL << 2)   /**< 1 = processor clock.       */

/* --- Interrupt Control and State Register (ICSR) --- */

/** ICSR at SCB offset 0xD04.
 *  Writing PENDSVSET (bit 28) triggers a PendSV exception.
 *  PendSV is used for context switching because it is deferrable:
 *  it only runs when no other ISR is active (set to lowest priority). */
#define SCB_ICSR    (*(volatile uint32_t *)0xE000ED04UL)
#define ICSR_PENDSVSET  (1UL << 28)

/* --- System Handler Priority Register 3 (SHPR3) --- */

/** SHPR3 at SCB offset 0xD20.
 *  Bits [23:16] = PendSV priority.
 *  Bits [31:24] = SysTick priority.
 *  Both set to 0xFF (lowest) so context switches never preempt ISRs. */
#define SCB_SHPR3   (*(volatile uint32_t *)0xE000ED20UL)

/* --- Stack frame constants --- */

/** Initial xPSR value.  Bit 24 = Thumb state.  Must always be set on
 *  Cortex-M; executing ARM instructions causes a UsageFault. */
#define XPSR_INIT_VALUE     0x01000000UL

/** EXC_RETURN for Thread mode, PSP, NO FPU frame.
 *  Bit 3 = 1 → return to Thread mode.
 *  Bit 2 = 1 → use PSP (not MSP).
 *  Bit 4 = 1 → standard frame (no FPU context stacked).
 *  On Cortex-M33 the full value is 0xFFFFFFFD. */
#define EXC_RETURN_THREAD_PSP   0xFFFFFFFDUL

/* ======================================================================
 * Kernel State — all statically allocated
 * ====================================================================== */

/** Currently running task.  Written by PendSV_Handler (ASM). */
OS_TCB_t *volatile os_current_task;

/** Next task selected by the scheduler.  Read by PendSV_Handler (ASM). */
OS_TCB_t *volatile os_next_task;

/** Global tick counter.  Incremented every 1 ms by SysTick_Handler. */
static volatile uint32_t os_tick_count;

/** Priority bitmap.  Bit (31 − prio) is set when at least one task at
 *  that priority is in the ready list.
 *  Highest-priority ready level = __CLZ(os_ready_bitmap). */
static volatile uint32_t os_ready_bitmap;

/** Per-priority ready list heads.  Each non-NULL entry points to the
 *  head of a circular singly-linked list of TCBs.  Round-robin is
 *  implemented by rotating the head after each time-slice. */
static OS_TCB_t *os_ready_list[OS_MAX_PRIO];

/** Task registry — flat array for tick processing (delay expiry scan). */
static OS_TCB_t *os_task_table[OS_MAX_TASKS];
static uint32_t  os_task_count;

/** Critical section nesting counter.  Supports nested EnterCritical. */
static volatile uint32_t os_critical_nesting;

/** Idle task TCB — always present at lowest priority (OS_MAX_PRIO − 1). */
static OS_TCB_t os_idle_tcb;

/* ======================================================================
 * Forward Declarations
 * ====================================================================== */

static void os_idle_task(void *arg);
static void os_task_exit_error(void);
static void os_ready_list_insert(OS_TCB_t *tcb);
static void os_ready_list_remove(OS_TCB_t *tcb);
static uint32_t *os_stack_init(OS_TCB_t *tcb,
                               void (*entry)(void *),
                               void *arg);

/* ======================================================================
 * Ready-List Management (Priority Bitmap + Circular Linked Lists)
 *
 * Bitmap encoding:
 *   bit position = 31 − priority
 *   Example: priority 0 (highest) → bit 31 (MSB)
 *            priority 31 (lowest) → bit 0  (LSB)
 *
 *   __CLZ(bitmap) returns the number of leading zero bits.
 *   For a bitmap with only bit 31 set, __CLZ = 0 → priority 0.
 *   For a bitmap with only bit  0 set, __CLZ = 31 → priority 31.
 *   Result: highest_ready_priority = __CLZ(bitmap).
 * ====================================================================== */

/**
 * @brief  Insert a TCB into the ready list at its priority level.
 *         Sets the corresponding bitmap bit.
 */
static void os_ready_list_insert(OS_TCB_t *tcb)
{
    uint32_t prio = tcb->priority;
    uint32_t bit  = 31U - prio;          /* bitmap bit position */

    if (os_ready_list[prio] == (OS_TCB_t *)0) {
        /* First task at this priority — create self-loop. */
        tcb->next = tcb;
        os_ready_list[prio] = tcb;
    } else {
        /* Insert after the current head (becomes second in ring).
         * This ensures the existing head runs next (fairness). */
        tcb->next = os_ready_list[prio]->next;
        os_ready_list[prio]->next = tcb;
    }

    /* Set the bitmap bit for this priority level. */
    os_ready_bitmap |= (1UL << bit);
}

/**
 * @brief  Remove a TCB from the ready list at its priority level.
 *         Clears the bitmap bit if the list becomes empty.
 */
static void os_ready_list_remove(OS_TCB_t *tcb)
{
    uint32_t prio = tcb->priority;
    uint32_t bit  = 31U - prio;

    if (tcb->next == tcb) {
        /* Only task at this priority — list becomes empty. */
        os_ready_list[prio] = (OS_TCB_t *)0;
        os_ready_bitmap &= ~(1UL << bit);
    } else {
        /* Find predecessor in the circular list. */
        OS_TCB_t *prev = tcb;
        while (prev->next != tcb) {
            prev = prev->next;
        }
        prev->next = tcb->next;

        /* If removing the head, advance it. */
        if (os_ready_list[prio] == tcb) {
            os_ready_list[prio] = tcb->next;
        }
    }

    tcb->next = (OS_TCB_t *)0;
}

/* ======================================================================
 * Stack Frame Initialisation
 *
 * Builds a fake exception-return frame so that the first PendSV (or
 * SVC for the very first task) pops registers and jumps to the entry
 * point as if returning from an interrupt.
 *
 * Stack layout (growing downward, lowest address at bottom):
 *
 *   [SP after init] → R4          ← manually restored by PendSV
 *                     R5
 *                     R6
 *                     R7
 *                     R8
 *                     R9
 *                     R10
 *                     R11
 *                     LR (EXC_RETURN = 0xFFFFFFFD)
 *                     ---- hardware auto-restored on exception return ----
 *                     R0  (= arg)
 *                     R1
 *                     R2
 *                     R3
 *                     R12
 *                     LR  (= task_exit_error)
 *                     PC  (= entry point)
 *                     xPSR (= 0x01000000, Thumb bit)
 *   [Top of stack]
 * ====================================================================== */

static void os_task_exit_error(void)
{
    /* Tasks must never return.  If they do, trap here for debugger. */
    __asm volatile ("cpsid i");
    for (;;) {
        __asm volatile ("nop");
    }
}

static uint32_t *os_stack_init(OS_TCB_t *tcb,
                               void (*entry)(void *),
                               void *arg)
{
    uint32_t *sp = &tcb->stack[OS_STACK_SIZE_WORDS];

    /* === Hardware-stacked exception frame (8 words) === */
    *(--sp) = XPSR_INIT_VALUE;              /* xPSR: Thumb bit set        */
    *(--sp) = (uint32_t)(uintptr_t)entry;   /* PC:   task entry point     */
    *(--sp) = (uint32_t)(uintptr_t)&os_task_exit_error; /* LR: trap      */
    *(--sp) = 0x00000000UL;                 /* R12                        */
    *(--sp) = 0x00000000UL;                 /* R3                         */
    *(--sp) = 0x00000000UL;                 /* R2                         */
    *(--sp) = 0x00000000UL;                 /* R1                         */
    *(--sp) = (uint32_t)(uintptr_t)arg;     /* R0:   first argument       */

    /* === Software-saved frame (9 words, matches PendSV LDMIA order) === */
    *(--sp) = EXC_RETURN_THREAD_PSP;        /* LR (EXC_RETURN)            */
    *(--sp) = 0x00000000UL;                 /* R11                        */
    *(--sp) = 0x00000000UL;                 /* R10                        */
    *(--sp) = 0x00000000UL;                 /* R9                         */
    *(--sp) = 0x00000000UL;                 /* R8                         */
    *(--sp) = 0x00000000UL;                 /* R7                         */
    *(--sp) = 0x00000000UL;                 /* R6                         */
    *(--sp) = 0x00000000UL;                 /* R5                         */
    *(--sp) = 0x00000000UL;                 /* R4                         */

    return sp;
}

/* ======================================================================
 * Idle Task
 *
 * Runs at the lowest priority (OS_MAX_PRIO − 1).  Executes WFI (Wait
 * For Interrupt) to reduce power consumption until the next SysTick.
 * ====================================================================== */

static void os_idle_task(void *arg)
{
    (void)arg;
    for (;;) {
        __asm volatile ("wfi");
    }
}

/* ======================================================================
 * Kernel API Implementation
 * ====================================================================== */

void OS_Init(void)
{
    uint32_t i;

    os_current_task    = (OS_TCB_t *)0;
    os_next_task       = (OS_TCB_t *)0;
    os_tick_count      = 0U;
    os_ready_bitmap    = 0U;
    os_critical_nesting = 0U;
    os_task_count      = 0U;

    for (i = 0U; i < OS_MAX_PRIO; i++) {
        os_ready_list[i] = (OS_TCB_t *)0;
    }
    for (i = 0U; i < OS_MAX_TASKS; i++) {
        os_task_table[i] = (OS_TCB_t *)0;
    }

    /* Create the idle task at the lowest priority level. */
    (void)memset(&os_idle_tcb, 0, sizeof(os_idle_tcb));
    (void)OS_Task_Create(&os_idle_tcb,
                         os_idle_task,
                         (void *)0,
                         OS_MAX_PRIO - 1U,
                         "idle");
}

int32_t OS_Task_Create(OS_TCB_t   *tcb,
                       void       (*entry)(void *),
                       void        *arg,
                       uint32_t     priority,
                       const char  *name)
{
    if ((tcb == (OS_TCB_t *)0) || (entry == (void (*)(void *))0)) {
        return -1;
    }
    if (priority >= OS_MAX_PRIO) {
        return -1;
    }
    if (os_task_count >= OS_MAX_TASKS) {
        return -1;
    }

    OS_EnterCritical();

    tcb->priority    = priority;
    tcb->state       = OS_TASK_READY;
    tcb->delay_ticks = 0U;
    tcb->name        = name;
    tcb->next        = (OS_TCB_t *)0;

    /* Build the initial stack frame. */
    tcb->sp = os_stack_init(tcb, entry, arg);

    /* Register in the global task table (for tick scan). */
    os_task_table[os_task_count] = tcb;
    os_task_count++;

    /* Add to the ready list. */
    os_ready_list_insert(tcb);

    OS_ExitCritical();

    return 0;
}

void OS_Start(void)
{
    uint32_t highest_prio;

    /* ------------------------------------------------------------------
     * 1. Set PendSV and SysTick to the LOWEST interrupt priority.
     *
     *    SHPR3 (0xE000ED20):
     *      Bits [23:16] = PendSV priority   → 0xFF
     *      Bits [31:24] = SysTick priority  → 0xFF
     *
     *    This guarantees that context switching (PendSV) never preempts
     *    any application ISR.  SysTick at low priority ensures the
     *    scheduler runs after all higher-priority work completes.
     * ------------------------------------------------------------------ */
    SCB_SHPR3 = 0xFFFF0000UL;

    /* ------------------------------------------------------------------
     * 2. Configure SysTick for 1 ms tick (200 MHz ICLK).
     *
     *    RVR = 199 999  →  (199 999 + 1) / 200 000 000 = 1 ms
     *    CVR = 0        →  Writing any value clears counter + COUNTFLAG
     *    CSR = ENABLE | TICKINT | CLKSOURCE (processor clock)
     * ------------------------------------------------------------------ */
    SYST_RVR = OS_SYSTICK_RELOAD;
    SYST_CVR = 0U;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;

    /* ------------------------------------------------------------------
     * 3. Select the highest-priority ready task as the first to run.
     * ------------------------------------------------------------------ */
    if (os_ready_bitmap == 0U) {
        /* No tasks created — should never happen if OS_Init was called. */
        for (;;) { __asm volatile ("nop"); }
    }

    highest_prio    = (uint32_t)__builtin_clz(os_ready_bitmap);
    os_current_task = os_ready_list[highest_prio];
    os_next_task    = os_current_task;
    os_current_task->state = OS_TASK_RUNNING;

    /* ------------------------------------------------------------------
     * 4. Launch the first task via SVC.
     *
     *    SVC transitions us to Handler mode so that we can perform
     *    a proper exception return (BX LR with EXC_RETURN value).
     *    The SVC_Handler in port_cm33.S loads the task's stack,
     *    sets PSP, switches CONTROL to use PSP, and returns to the
     *    task entry point.
     * ------------------------------------------------------------------ */
    OS_StartFirstTask();   /* Implemented in port_cm33.S — never returns */

    /* Should never reach here. */
    for (;;) { __asm volatile ("nop"); }
}

void OS_Schedule(void)
{
    uint32_t highest_prio;
    OS_TCB_t *candidate;

    if (os_ready_bitmap == 0U) {
        return;   /* Only possible if idle task was removed (bug). */
    }

    /* O(1) lookup: find the highest-priority non-empty ready list. */
    highest_prio = (uint32_t)__builtin_clz(os_ready_bitmap);
    candidate    = os_ready_list[highest_prio];

    if (candidate != os_current_task) {
        /* Different task selected — trigger context switch via PendSV. */
        os_next_task = candidate;
        OS_TriggerPendSV();
    }
}

void OS_Task_Delay(uint32_t ticks)
{
    if (ticks == 0U) {
        return;
    }

    OS_EnterCritical();

    os_current_task->delay_ticks = ticks;
    os_current_task->state       = OS_TASK_BLOCKED;

    /* Remove from ready list — may clear bitmap bit. */
    os_ready_list_remove(os_current_task);

    OS_ExitCritical();

    /* Trigger reschedule.  PendSV will switch to the next ready task. */
    OS_Schedule();
}

void OS_Yield(void)
{
    OS_EnterCritical();

    /* Rotate the round-robin ring at the current priority level.
     * Advancing the head means the next task in the ring runs next. */
    uint32_t prio = os_current_task->priority;
    if ((os_ready_list[prio] != (OS_TCB_t *)0) &&
        (os_ready_list[prio]->next != os_ready_list[prio])) {
        os_ready_list[prio] = os_ready_list[prio]->next;
    }

    OS_ExitCritical();

    OS_Schedule();
}

uint32_t OS_GetTick(void)
{
    return os_tick_count;
}

void OS_EnterCritical(void)
{
    __asm volatile ("cpsid i" ::: "memory");
    os_critical_nesting++;
}

void OS_ExitCritical(void)
{
    os_critical_nesting--;
    if (os_critical_nesting == 0U) {
        __asm volatile ("cpsie i" ::: "memory");
    }
}

void OS_TriggerPendSV(void)
{
    SCB_ICSR = ICSR_PENDSVSET;
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");
}

/* ======================================================================
 * SysTick_Handler — Kernel Heartbeat (1 ms)
 *
 * Overrides the weak alias in startup.c.  This function:
 *   1. Increments the global tick counter.
 *   2. Scans all blocked tasks — decrements delay_ticks.
 *   3. Moves expired tasks (delay_ticks == 0) back to the ready list.
 *   4. Performs Round-Robin rotation at the current priority level.
 *   5. Calls OS_Schedule() to trigger PendSV if needed.
 * ====================================================================== */

void SysTick_Handler(void)
{
    uint32_t i;
    OS_TCB_t *tcb;
    uint32_t  current_prio;

    os_tick_count++;

    /* --- Scan blocked tasks for delay expiry --- */
    for (i = 0U; i < os_task_count; i++) {
        tcb = os_task_table[i];
        if ((tcb != (OS_TCB_t *)0) && (tcb->state == OS_TASK_BLOCKED)) {
            if (tcb->delay_ticks > 0U) {
                tcb->delay_ticks--;
            }
            if (tcb->delay_ticks == 0U) {
                tcb->state = OS_TASK_READY;
                os_ready_list_insert(tcb);
            }
        }
    }

    /* --- Round-Robin rotation at the current running priority --- */
    if (os_current_task != (OS_TCB_t *)0) {
        current_prio = os_current_task->priority;
        if ((os_ready_list[current_prio] != (OS_TCB_t *)0) &&
            (os_ready_list[current_prio]->next != os_ready_list[current_prio])) {
            /* Rotate: advance the head pointer so the next task in the
             * ring will be selected by OS_Schedule(). */
            os_ready_list[current_prio] = os_ready_list[current_prio]->next;
        }
    }

    /* --- Run the scheduler (may trigger PendSV) --- */
    OS_Schedule();
}
