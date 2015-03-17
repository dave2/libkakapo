#ifndef SCHED_SIMPLE_H_INCLUDED
#define SCHED_SIMPLE_H_INCLUDED

/* Copyright (C) 2015 David Zanetti
 *
 * This file is part of libkakapo
 *
 * libkakapo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * libkapao is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libkapapo.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 *  \brief FIFO scheduler based on a ring buffer of tasks to execute
 *
 * Based on a design provided by phirate
 * No preempt, no context switching, co-operative
 *
 * A "task" in this schedular is a function that must run to completion
 * before another task may be run. Each task must maintain whatever state
 * it requires between executes.
 *
 * There is no yield(), instead the function should update it's own state
 * to a suitable place to enter again when called, and return.
 *
 * Task functions have only one argument: a pointer to some data that may
 * be useful. This pointer may be NULL, so should be checked. No guarantees
 * are made about the volatile nature of the data this is pointed to.
 * While you could use this as an explcit int by casting it, this is not
 * recommended, and instead you should have a static int somewhere else and
 * pass the pointer to it.
 *
 * Calling sched_run() on a task adds that function with associated data
 * pointer to the run queue. The run queue has a fixed size, set by the
 * sched_simple_init() function. However, there is no limit to the number
 * of possible tasks which may execute on the system. That is, only the
 * pending tasks actually told to run will count against this limit.
 *
 * A task cannot be destroyed. A task may call itself to be run again.
 *
 * sched_run() accepts two priorities, sched_later which places the task
 * at the end of the run queue, and sched_now which placed it at the top.
 * There are no other priority levels.
 */

/** \brief Task priority for being added to run queue */
typedef enum {
    sched_now = 0,  /**< Task runs immediately */
    sched_later     /**< Task runs some other time */
} sched_prio_t;

/** \brief Initalise the scheduler
 *
 *  Must be called before any other scheduling functions!
 *
 *  \param qlen The length of the task run queue
 */
int sched_simple_init(uint8_t qlen);

/** \brief Execute all tasks in the queue.
 *
 *  Note: during a task, further tasks may be added to the FIFO, therefore
 *  this function only returns when no tasks are able to be executed.
 *  The main loop should probably perform a sleep, or similar, but it safe
 *  to call this even if no tasks have been set to run
 */
void sched_simple(void);

/**< \brief Add a task to the run queue
 *
 *  With sched_now, it will be added to the head of the run queue.
 *  sched_later adds it to the tail. It may never get executed from the
 *  tail, if more tasks of sched_now are added.
 *
 *  \param fn Pointer to the function to run as a task
 *  \param data Pointer to the data to use for this invocation
 *  \param prio Priority to add the task at
 */
int sched_run(void(*fn)(void *), void *data, sched_prio_t prio);

#endif // SCHED_SIMPLE_H_INCLUDED
