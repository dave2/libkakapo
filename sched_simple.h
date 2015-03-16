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
 *  \brief A simple FIFO task scheduler
 *
 *  Allows the creation and run of 'tasks' on a FIFO basis. Tasks can be
 *  added to the run queue either at the tail (sched_later prio), or at
 *  the head (sched_now prio). A task is a function that must return which
 *  maintains it's own state.
 *
 *  This scheduler does not implement preempt or context switching. A task
 *  must return to ensure the system does not lock. You are strongly suggested
 *  to run this with the watchdog timer to catch cases where the system locks.
 *  A task must also explicitly schedule it's own re-run if that is required.
 *
 *  A task may have private data associated with it, for maintaining state.
 *  Creation and maint of the private data is the responsibility of the task.
 *
 *  Since this is a simple FIFO, there are no assurances of starvation from
 *  a sched_now task constantly putting itself on top.
 *
 *  The advantage of a simple FIFO is memory impact is light, and run-time
 *  cost is relatively low.
 *
 *  Note: tasks can only be created, not destroyed.
 */

/** \brief Process table structure */
typedef struct task_struct {
    volatile struct task_struct *next; /**< Next task in linked list */
    volatile struct task_struct *prev; /**< Previous task in linked list */
    void (*fn)(struct task_struct *); /**< Pointer to the actual task entry point */
    void *data; /**< Task's own private data */
} task_t;

/** \brief Task priority for being added to run queue */
typedef enum {
    sched_now = 0,  /**< Task runs immediately */
    sched_later     /**< Task runs some other time */
} sched_prio_t;

/** \brief Initalise the scheduler
 *
 *  Must be called before any other scheduling functions!
 */
void sched_simple_init(void);

/** \brief Execute all tasks in the queue.
 *
 *  Note: during a task, further tasks may be added to the FIFO, therefore
 *  this function only returns when no tasks are able to be executed.
 *  The main loop should probably perform a sleep, or similar, but it safe
 *  to call this even if no tasks have been set to run
 */
void sched_simple(void);

/** \brief Create a new task
 *
 *  The task must consist of a function that does not have a return value
 *  and accepts one argument: a pointer to the task for the scheduler to
 *  operate on. Functions MUST return to ensure that anything else gets
 *  executed. Creating a task does not inject it into the run queue.
 *
 *  \param fn Function to use for task. Must not be NULL.
 *  \return task structure pointer, NULL if creation failed
 */
task_t *sched_create(void (*fn)(task_t *));

/**< \brief Add a task to the run queue
 *
 *  With sched_now, it will be added to the head of the run queue.
 *  sched_later adds it to the tail. It may never get executed from the
 *  tail, if more tasks of sched_now are added.
 *
 *  \param task Pointer to the task to change run state on
 *  \param prio Priority to add the task at
 */
void sched_run(task_t *task, sched_prio_t prio);

#endif // SCHED_SIMPLE_H_INCLUDED
