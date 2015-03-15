/* Copyright (C) 2015 David Zanetti
 *
 * This file is part of libkakapo
 *
 * libkakapo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2 of the
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

/* FIFO scheduler with no context switching and limited priority */

/* A "task" in this scheduler is a function that must run to completion
 * before another task may be run. Each task is therefore a function and
 * any state must be maintained by the function itself.
 *
 * There is no yield(), instead the function should update it's own state
 * and return.
 *
 * Task functions have only one argument, the pid of the task for use in
 * changing run-state, destroying the task, and other cases. Once a task
 * has executed once, it will not execute again until an explicit call to
 * run the task is made. A task can trigger iself to run.
 */

/* since there is no context switching, there is no TCB and memory footprint
 * is largely determined by private state within tasks and 8 bytes per execute
 * slot in the process table. */

/* Internally, two pointers maintain the FIFO list, and a pointer is
 * kept to the tail and head. Inserting at head forces the next task to
 * be a high-priority, inserting at tail joins the end of the FIFO.
 * Interrupts are disabled while pointers are updated */

/* We use linked lists since that allows O(1) task selection, rather than
 * based on the size of the process table to find another task */

#include <avr/io.h>
#include "global.h"
#include <stdio.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>
#include "sched_simple.h"
#include "errors.h"

/**< Maximum number of execute slots supported */
#define SCHED_SIMPLE_MAXPROC 16

/**< the fixed process table */
task_t _proc[SCHED_SIMPLE_MAXPROC];
/**< head and tail pointers for the linked list */
task_t *_head;
task_t *_tail;
/**< glue is used to provide next pointer when current task is pushed to tail */
task_t glue;

void sched_simple_init(void) {
    /* reset pointers for task selection */
    _head = NULL;
    _tail = NULL;
    /* reset the whole process table */
    memset(&_proc,0,sizeof(task_t)*SCHED_SIMPLE_MAXPROC);
    /* and glue task */
    memset(&glue,0,sizeof(task_t));
    return;
}

void sched_simple(void) {
    /* while _head is not NULL, execute that task, then work out next task */
    while (_head) {
        /* sanity check */
        if (_head->fn) {
            (_head->fn)(_head); /* Execute the task */
            /* select a new task */
            /* Must be executed without interrupts going off, since we are
             * doing a 16-bit copy here */
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                /* change the head pointer to this task's next */
                /* this may have been changed by the previous task */
                _head = _head->next;
                if (_head) {
                    _head->prev = NULL; /* we are head */
                } else {
                    _tail = NULL; /* make sure tail is reset when we run out */
                }
            }
        }
    }
    /* nothing left to run */
    return;
}

task_t *sched_create(void (*fn)(task_t *task)) {
    uint8_t n;

    if (!fn) {
        return NULL; /* if a task doesn't have a function, then it doesn't exist */
    }

    /* find a slot to execute the task in */
    for (n = 0; n < SCHED_SIMPLE_MAXPROC; n++) {
        if (_proc[n].fn == NULL) {
            /* looks clear, allocate it (should be safe with interupts on) */
            _proc[n].fn = fn;
            _proc[n].data = NULL; /* clear any private data */
            _proc[n].next = NULL; /* has no run queue pointers */
            _proc[n].prev = NULL;
            /* return the structure */
            return &(_proc[n]);
        }
    }

    return NULL; /* no space to allocate this task, sorry */
}

void sched_run(task_t *task, sched_prio_t prio) {

    /* this all has to be done without interupts breaking our pointers */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        switch (prio) {
            case sched_later:
                /* if we are the current head, then we want to run again later */
                if (_head == task) {
                    /* set up some glue to make this work */
                    glue.next = task->next;
                    /* point the head to the glue, containing original next task */
                    _head = &glue;
                }
                /* okay, make us the tail of the run queue */
                task->next = NULL; /* make sure we're the end */
                /* if we have no tail, we can't very well set it's next */
                if (_tail) {
                    _tail->next = task; /* current tail's next is us */
                }
                task->prev = _tail; /* previous from us is current tail */
                _tail = task; /* we are now tail */
                break;
            case sched_now:
                /* make glue point to us */
                glue.next = task;
                /* if we weren't the head, then make our next point to head */
                /* if we are the head, then our existing next is acceptable */
                if (_head != task) {
                    task->next = _head;
                }
                /* we have no previous task */
                task->prev = NULL;
                /* make glue head, which makes us head on next pass */
                _head = &glue;
            default:
                break; /* nothing much we can do */
        }
    }

    return;
}

