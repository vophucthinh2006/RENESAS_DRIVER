/**
 * @file    kernel.h
 * @brief   Preemptive RTOS Kernel for Renesas RA6M5 (Cortex-M33)
 *
 * Architecture:
 *   - Priority-based preemption with Round-Robin at equal priority levels.
 *   - O(1) scheduling via a 32-bit priority bitmap and __CLZ.
 *   - Static memory only — no malloc/new.
 *   - Zero FSP dependency — direct register access via CMSIS headers.
 *
 * MISRA C:2012 compliance where possible.  Deviations documented inline.
 *
 * @note    The 'sp' field MUST remain at offset 0 in OS_TCB_t.
 *          The PendSV assembly (port_cm33.S) relies on this layout.
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

/* ======================================================================
 * Kernel Configuration
 * ====================================================================== */

/** Maximum number of concurrent tasks (including idle task). */
#define OS_MAX_TASKS            32U

/** Number of discrete priority levels (0 = highest, 31 = lowest). */
#define OS_MAX_PRIO             32U

/** Per-task stack size in 32-bit words.  4096 bytes = 1024 words.
 *  Sized for FOTA and TinyML/AI workloads. */
#define OS_STACK_SIZE_WORDS     1024U

/** Kernel tick rate.  1 ms tick at 200 MHz ICLK (PLL). */
#define OS_TICK_RATE_HZ         1000U

/** SysTick reload value.  ICLK / OS_TICK_RATE_HZ - 1 = 199999. */
#define OS_SYSTICK_RELOAD       199999U

/* ======================================================================
 * Task States
 * ====================================================================== */

typedef enum {
    OS_TASK_READY     = 0U,   /**< In the ready list, eligible to run.       */
    OS_TASK_RUNNING   = 1U,   /**< Currently executing on the CPU.           */
    OS_TASK_BLOCKED   = 2U,   /**< Waiting (e.g. OS_Task_Delay).             */
    OS_TASK_SUSPENDED = 3U    /**< Removed from scheduling until resumed.    */
} OS_TaskState_t;

/* ======================================================================
 * Task Control Block (TCB)
 *
 * CRITICAL LAYOUT CONSTRAINT:
 *   'sp' MUST be the first field (offset 0).
 *   The PendSV handler in port_cm33.S loads/stores the stack pointer
 *   using:  LDR r0, [r2]  /  STR r0, [r2]
 *   where r2 points to the TCB.  Any change breaks context switching.
 * ====================================================================== */

typedef struct OS_TCB {
    uint32_t          *sp;                        /**< Saved stack pointer (OFFSET 0). */
    struct OS_TCB     *next;                      /**< Next TCB in round-robin ring.   */
    uint32_t           priority;                  /**< 0 = highest, 31 = lowest.       */
    OS_TaskState_t     state;                     /**< Current task state.              */
    uint32_t           delay_ticks;               /**< Remaining delay ticks.           */
    const char        *name;                      /**< Human-readable name (debug).     */
    uint32_t           stack[OS_STACK_SIZE_WORDS]; /**< Static stack storage.            */
} OS_TCB_t;

/* ======================================================================
 * Kernel Globals (defined in kernel.c, used by port_cm33.S)
 * ====================================================================== */

/** Currently executing task.  Read/written by PendSV (ASM). */
extern OS_TCB_t *volatile os_current_task;

/** Task selected by the scheduler to run next.  Read by PendSV (ASM). */
extern OS_TCB_t *volatile os_next_task;

/* ======================================================================
 * Kernel API
 * ====================================================================== */

/**
 * @brief  Initialise the kernel.  Must be called before any other OS_ call.
 *         Zeroes all internal state and creates the idle task.
 */
void OS_Init(void);

/**
 * @brief  Create a task from a statically allocated TCB.
 *
 * @param  tcb       Pointer to a caller-provided static OS_TCB_t.
 * @param  entry     Task entry point.  Signature: void task(void *arg).
 * @param  arg       Argument passed to the task via R0.
 * @param  priority  0 (highest) .. OS_MAX_PRIO-1 (lowest).
 * @param  name      Null-terminated name string (for debug only).
 * @return 0 on success, -1 on error (invalid priority / table full).
 */
int32_t OS_Task_Create(OS_TCB_t *tcb,
                       void (*entry)(void *),
                       void     *arg,
                       uint32_t  priority,
                       const char *name);

/**
 * @brief  Start the kernel.  Configures SysTick, sets NVIC priorities,
 *         and launches the highest-priority ready task.
 *         This function never returns.
 */
void OS_Start(void);

/**
 * @brief  Delay the calling task for a specified number of ticks.
 *         The task is moved to BLOCKED state and rescheduled.
 *
 * @param  ticks  Number of system ticks to delay (1 tick = 1 ms).
 */
void OS_Task_Delay(uint32_t ticks);

/**
 * @brief  Voluntarily yield the CPU to the next ready task.
 */
void OS_Yield(void);

/**
 * @brief  Run the scheduler.  Called from SysTick_Handler.
 *         If a different task is selected, PendSV is triggered.
 */
void OS_Schedule(void);

/**
 * @brief  Return the current kernel tick count (incremented every 1 ms).
 */
uint32_t OS_GetTick(void);

/**
 * @brief  Enter a critical section (disable interrupts).
 *         Supports nesting.
 */
void OS_EnterCritical(void);

/**
 * @brief  Exit a critical section.  Re-enables interrupts only when
 *         the outermost critical section exits.
 */
void OS_ExitCritical(void);

/* ======================================================================
 * Port-layer functions (implemented in port_cm33.S / kernel.c)
 * ====================================================================== */

/** Trigger PendSV by setting PENDSVSET in ICSR. */
void OS_TriggerPendSV(void);

/** Assembly routine: launch the very first task via SVC. */
extern void OS_StartFirstTask(void);

#endif /* KERNEL_H */
