/* Copyright (C) 2009-2014 David Zanetti
 *
 * This file is part of libkakapo.
 *
 * libkakapo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
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

/* ringbuffer handler */

/** \file
 *  \brief Ringbuffer implementation
 *
 *  Ringbuffers are circular buffer which can be read from or written to,
 *  with independant read and write pointers. They must be sized by a power
 *  of two, since this implementation uses simple bit masking to implement
 *  wrapping.
 *
 *  Functions come in normal and 'unsafe' version. In the unsafe versions you
 *  must ensure that no concurrent access to a single ringbuffer is possible.
 *  The normal (safe) versions implement this with global interrupt disable,
 *  which is blunt but effective.
 *
 *  Note: the current limit for the size of a ringbuffer is 256 bytes.
 */

#include <stdio.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdlib.h>
#include "ringbuffer.h"

/* create a ringbuffer, allocating the appropriate space for it's metadata and ring size */
/* must be a power of two */

ringbuffer_t *ring_create(uint16_t len) {
	ringbuffer_t *ring = NULL;

    /* check to see length is power of 2 and not excessive */
    if (len > RINGBUFFER_MAX || len & (len-1)) {
        /* not a power of two */
        return NULL;
    }

	/* attempt to get some memory for this ringbuffer */
	ring = malloc(sizeof(ringbuffer_t));
	if (!ring) {
		/* we failed, return NULL and hope calling code can deal with this */
		return NULL;
	}

	/* init the metadata, first the pointers */
	ring->head = 0;
	ring->tail = 0;
	ring->mask = len-1; /* since this is a power of two, 1 less is the mask */
	/* now attempt to malloc the space */
	ring->buf = malloc(len);
	if (!ring->buf) {
		free(ring); /* failed! free what we have created so far, give up */
		return NULL;
	}

	return ring;
}

/* reset the contents of a ringbuffer */
void ring_reset(ringbuffer_t *ring) {
	ring->head = 0;
	ring->tail = 0;
	return;
}

void ring_destroy(ringbuffer_t *ring) {
	/* let's not just attempt to free something unallocated */
	if (!ring) {
		return;
	}
	/* free the buffer associated with us */
	if (ring->buf) {
		free(ring->buf);
	}
	/* and our struct */
	free(ring);
	return;
}

/* write to the given ring buffer, assumes it is safe to do so */
uint8_t ring_write(ringbuffer_t *ring, char value) {
	uint8_t ret;
	//ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = ring_write_unsafe(ring, value);
	//};

	return ret;
}

uint8_t ring_write_unsafe(ringbuffer_t *ring, char s) {
	//PORTE.OUTTGL = PIN6_bm;
	if (((ring->head + 1) & ring->mask) != ring->tail) {
		ring->head = (ring->head + 1) & ring->mask;
		*(ring->buf + ring->head) = s;
	} else {
		return 0;
	}
	return 1;
}

char ring_read(ringbuffer_t *ring) {
	char ret;
//	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = ring_read_unsafe(ring);
	//};
	return ret;
}

char ring_read_unsafe(ringbuffer_t *ring) {
	//PORTE.OUTTGL = PIN5_bm;
	ring->tail = (ring->tail + 1) & ring->mask;
	return *(ring->buf + ring->tail);
}

uint8_t ring_readable(ringbuffer_t *ring) {
	uint8_t ret;
//	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = ring_readable_unsafe(ring);
//	};
	return ret;
}

uint8_t ring_readable_unsafe(ringbuffer_t *ring) {
	if (ring->tail != ring->head) {
		return 1;
	}
	return 0;
}

