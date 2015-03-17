/* Copyright (C) 2015 David Zanetti
 *
 * This file is part of libkakapo
 *
 * libkakapo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3 of the
 * License.
 *
 * libkakapo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libkapapo.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

/* FIFO scheduler based on a ring buffer of tasks to execute
 * Based on a design provided by phirate
 * No preempt, no context switching, co-operative
 */

/* A "task" in this schedular is a function that must run to completion
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

#include <avr/io.h>
#include "global.h"
#include <stdio.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>
#include "errors.h"
#include "debug.h"

#include "sched_simple.h"

typedef struct {
    void (*fn)(void *); /**< Pointer to the actual task entry point */
    void *data; /**< Private data for this task to use */
} task_t;

/**< Process table is defined by init, so we only have a buffer here */
task_t *_runq; /* this pointer can be cached since it doesn't change */
volatile task_t *_runq_start;
volatile task_t *_runq_end;
volatile uint8_t _runq_len;
volatile uint8_t _runq_entries;

int sched_simple_init(uint8_t size) {
    /* allocate space for the queue */
    _runq = malloc(sizeof(task_t)*size);
    if (!_runq) {
        k_debug("failed to allocate memory for runq");
        return -ENOMEM;
    }

    /* reset various pointers */
    _runq_start = _runq;
    _runq_end = _runq;
    _runq_entries = 0;
    _runq_len = size;

    /* reset the memory */
    memset(_runq,0,sizeof(task_t)*size);

    return 0;
}

/* rotate the ring pointer backwards */
/* MUST BE CALLED WITH INTERRUPTS OFF */
__attribute__((always_inline)) inline void _runq_back(task_t **p) {
    if (*p == _runq) {
        *p = _runq + sizeof(task_t) * _runq_len;
    } else {
        (*p)--;
    }
}

/* rotate the ring pointer forwards */
/* MUST BE CALLED WITH INTERRUPTS OFF */
__attribute__((always_inline)) inline void _runq_forward(task_t **p) {
    if (*p == _runq + sizeof(task_t) * _runq_len) {
        *p = _runq;
    } else {
        (*p)++;
    }
}

/* push a task on to the top of the stack */
/* THIS MUST BE CALLED WITH INTERRUPTS OFF */
int _runq_push_start(void (*fn)(void *),void *data) {
    /* check to see if we have anywhere to put this */
    if (_runq_entries == _runq_len) {
        return -ENOMEM;
    }
    /* push the current point backwards */
    _runq_back((task_t **)&_runq_start);
    /* update the number of tasks */
    _runq_entries++;
    /* update the runq entry we just made */
    _runq_start->fn = fn;
    _runq_start->data = data;

    return 0;
}

/* put a task on the bottom of the queue */
/* THIS MUST BE CALLED WITH INTERRUPTS OFF */
int _runq_push_end(void (*fn)(void *),void *data) {
    /* check to see if we have anywhere to put this */
    if (_runq_entries == _runq_len) {
        return -ENOMEM;
    }
    /* advance the end of the ring */
    _runq_forward((task_t **)&_runq_end);
    /* update entry count */
    _runq_entries++;
    /* update the runq entry we just made */
    _runq_end->fn = fn;
    _runq_end->data = data;
    return 0;
}

/* retrieve the top task on the runq */
/* THIS MUST BE CALLED WITH INTERUPTS OFF */
task_t *_runq_pop_start(void) {
    task_t *ret;
    /* if we're empty, then return nothing */
    if (!_runq_entries) {
        return NULL;
    }
    /* the top of the stack is what we're returning */
    ret = (task_t *)_runq_start;
    _runq_forward((task_t **)&_runq_start);
    _runq_entries--;
    /* return our, hopefully safe copy of the task */
    return ret;
}

/* wrappers to the various cases */
int sched_run(void (*fn)(void *),void *data,sched_prio_t prio) {
    switch (prio) {
        case sched_later:
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                return _runq_push_end(fn,data);
            }
            break;
        case sched_now:
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                return _runq_push_start(fn,data);
            }
            break;
        default:
            break;
    }

    /* something else, just don't bother */
    return -EINVAL;
}

/* the actual job schedular */
void sched_simple(void) {
    task_t task, *next = NULL;

    while (1) {
        /* we must retreive and copy the task with interrupts off */
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            next = _runq_pop_start();
            if (!next) {
                return; /* nothing more to do */
            }
            memcpy(&task,next,sizeof(task_t));
        }
        /* safe to now run the task */
        (task.fn)(task.data);
    }
}
