/**
 * @file    semaphore.c
 * @brief   Counting & Binary Semaphore Implementation
 *
 * Wait list is priority-ordered: OS_SemPost wakes the highest-priority
 * waiter first.  OS_SemPend supports timeout (OS_NO_WAIT, OS_WAIT_FOREVER,
 * or N ticks).  OS_SemPost is ISR-safe.
 */

#include "semaphore.h"
#include <string.h>

/* ======================================================================
 * Ready-list helpers (defined in kernel.c, not public API).
 * We re-declare them here as extern to avoid circular header deps.
 * ====================================================================== */

/* These internal functions are in kernel.c — we need access for
 * adding/removing tasks from the ready list when posting/pending. */

/* Forward: we use the public API OS_Schedule / OS_EnterCritical etc.
 * For ready-list insert/remove we duplicate minimal logic here to
 * avoid exposing kernel internals.  Instead, we manipulate task state
 * and let the kernel's SysTick pick up the changes, OR we directly
 * call OS_Schedule after state changes.
 *
 * Design choice: semaphore.c uses the kernel's public critical section
 * and schedule APIs, plus direct manipulation of the ready bitmap and
 * list through a small internal interface. */

/* --- Internal ready-list access (mirrors kernel.c statics) ---
 * These are defined in kernel.c.  We access them via extern because
 * both modules are part of the same RTOS — not a user-facing API. */
extern volatile uint32_t os_ready_bitmap;
extern OS_TCB_t         *os_ready_list[];

static void sem_ready_list_insert(OS_TCB_t *tcb)
{
    uint32_t prio = tcb->priority;
    uint32_t bit  = 31U - prio;

    if (os_ready_list[prio] == (OS_TCB_t *)0) {
        tcb->next = tcb;
        os_ready_list[prio] = tcb;
    } else {
        tcb->next = os_ready_list[prio]->next;
        os_ready_list[prio]->next = tcb;
    }
    os_ready_bitmap |= (1UL << bit);
}

static void sem_ready_list_remove(OS_TCB_t *tcb)
{
    uint32_t prio = tcb->priority;
    uint32_t bit  = 31U - prio;

    if (tcb->next == tcb) {
        os_ready_list[prio] = (OS_TCB_t *)0;
        os_ready_bitmap &= ~(1UL << bit);
    } else {
        OS_TCB_t *prev = tcb;
        while (prev->next != tcb) { prev = prev->next; }
        prev->next = tcb->next;
        if (os_ready_list[prio] == tcb) {
            os_ready_list[prio] = tcb->next;
        }
    }
    tcb->next = (OS_TCB_t *)0;
}

/* ======================================================================
 * Wait-List Helpers
 * ====================================================================== */

/** Insert a TCB into the semaphore's wait list (unsorted — we search
 *  for highest priority on post). */
static int32_t wait_list_add(Semaphore_t *sem, OS_TCB_t *tcb)
{
    if (sem->wait_count >= OS_SEM_MAX_WAITERS) {
        return OS_ERR_FULL;
    }
    sem->wait_list[sem->wait_count] = tcb;
    sem->wait_count++;
    return OS_OK;
}

/** Remove a specific TCB from the wait list (for timeout cleanup). */
static void wait_list_remove(Semaphore_t *sem, OS_TCB_t *tcb)
{
    uint32_t i;
    for (i = 0U; i < sem->wait_count; i++) {
        if (sem->wait_list[i] == tcb) {
            /* Shift remaining entries down. */
            uint32_t j;
            for (j = i; j < sem->wait_count - 1U; j++) {
                sem->wait_list[j] = sem->wait_list[j + 1U];
            }
            sem->wait_list[sem->wait_count - 1U] = (OS_TCB_t *)0;
            sem->wait_count--;
            return;
        }
    }
}

/** Find and remove the highest-priority (lowest prio number) waiter. */
static OS_TCB_t *wait_list_pop_highest(Semaphore_t *sem)
{
    if (sem->wait_count == 0U) { return (OS_TCB_t *)0; }

    uint32_t best_idx  = 0U;
    uint32_t best_prio = sem->wait_list[0]->priority;
    uint32_t i;

    for (i = 1U; i < sem->wait_count; i++) {
        if (sem->wait_list[i]->priority < best_prio) {
            best_prio = sem->wait_list[i]->priority;
            best_idx  = i;
        }
    }

    OS_TCB_t *tcb = sem->wait_list[best_idx];
    /* Remove by shifting. */
    uint32_t j;
    for (j = best_idx; j < sem->wait_count - 1U; j++) {
        sem->wait_list[j] = sem->wait_list[j + 1U];
    }
    sem->wait_list[sem->wait_count - 1U] = (OS_TCB_t *)0;
    sem->wait_count--;

    return tcb;
}

/* ======================================================================
 * Semaphore API
 * ====================================================================== */

int32_t OS_SemCreate(Semaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
    if ((sem == (Semaphore_t *)0) || (max_count == 0U)) {
        return OS_ERR_PARAM;
    }
    if (initial_count > max_count) {
        return OS_ERR_PARAM;
    }

    sem->count      = initial_count;
    sem->max_count  = max_count;
    sem->wait_count = 0U;
    (void)memset(sem->wait_list, 0, sizeof(sem->wait_list));

    return OS_OK;
}

int32_t OS_SemPend(Semaphore_t *sem, uint32_t timeout)
{
    if (sem == (Semaphore_t *)0) { return OS_ERR_PARAM; }

    OS_EnterCritical();

    /* Fast path: semaphore available. */
    if (sem->count > 0U) {
        sem->count--;
        OS_ExitCritical();
        return OS_OK;
    }

    /* Count is 0 — need to block (or return if no-wait). */
    if (timeout == OS_NO_WAIT) {
        OS_ExitCritical();
        return OS_ERR_TIMEOUT;
    }

    /* Block the calling task. */
    OS_TCB_t *self = os_current_task;

    if (wait_list_add(sem, self) != OS_OK) {
        OS_ExitCritical();
        return OS_ERR_FULL;
    }

    self->state        = OS_TASK_BLOCKED;
    self->blocked_on   = (void *)sem;
    self->block_result = OS_OK;
    self->delay_ticks  = timeout;  /* OS_WAIT_FOREVER or N ticks */

    sem_ready_list_remove(self);

    OS_ExitCritical();

    /* Trigger reschedule — PendSV will switch to next ready task. */
    OS_Schedule();

    /* --- Execution resumes here when unblocked --- */

    /* block_result set by OS_SemPost (OS_OK) or SysTick timeout handler
     * (OS_ERR_TIMEOUT). */
    return self->block_result;
}

int32_t OS_SemPost(Semaphore_t *sem)
{
    if (sem == (Semaphore_t *)0) { return OS_ERR_PARAM; }

    OS_EnterCritical();

    if (sem->wait_count > 0U) {
        /* Wake the highest-priority waiting task. */
        OS_TCB_t *waiter = wait_list_pop_highest(sem);

        waiter->state        = OS_TASK_READY;
        waiter->blocked_on   = (void *)0;
        waiter->block_result = OS_OK;
        waiter->delay_ticks  = 0U;

        sem_ready_list_insert(waiter);

        OS_ExitCritical();

        /* Trigger reschedule (safe from ISR — just sets PENDSVSET). */
        OS_Schedule();
    } else {
        /* No waiters — increment count (capped at max). */
        if (sem->count < sem->max_count) {
            sem->count++;
        }
        OS_ExitCritical();
    }

    return OS_OK;
}

void OS_Sem_TimeoutHandler(Semaphore_t *sem, OS_TCB_t *tcb)
{
    /* Called from SysTick_Handler (kernel.c) when a blocked task's
     * delay_ticks reaches 0 while blocked_on points to a semaphore.
     * Remove the task from the semaphore's wait list. */
    wait_list_remove(sem, tcb);
}
