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

/* Public interface to generic ringbuffer code */

#ifndef RINGBUFFER_H_INCLUDED
#define RINGBUFFER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  \brief Ringbuffer public interface
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

/* maximum size of a ringbuffer */
#define RINGBUFFER_MAX 256

/** \struct ringbuffer_t
 *  \brief Buffer and metadata for a ringbuffer
 */
typedef struct {
		char *buf;      /**< The actual ringbuffer storage */
		uint8_t head;   /**< Head pointer */
		uint8_t tail;   /**< Tail pointer */
		uint8_t mask;   /**< Mask to wrap ringbuffer */
} ringbuffer_t;

/** \brief Create a ringbuffer of the given mask
 *  \param len The length of the ringbuffer. This must be a power of two.
 *  \return Pointer to the created ringbuffer, NULL if is failed to allocate memory
 *  or some other error.
 */
ringbuffer_t *ring_create(uint16_t len);

/** \brief Reset (aka flush) the contents of a ringbuffer
 *  \param ring The ringbuffer to flush
 */
void ring_reset(ringbuffer_t *ring);

/** \brief Destroy a ringbuffer
 *
 *  Frees all resources allocated by it, including the metadata about the ring.
 *
 *  Note: use sparingly, free() may be a null call on your platform!
 *
 *  \param ring The ringbuffer to destroy
 */
void ring_destroy(ringbuffer_t *ring);

/** \brief Write a character to the ringbuffer
 *
 *  Note: this disables global interrupts. You may wish to use the unsafe
 *  versions instead, and selectively disable interrupts.
 *
 *  \param ring Ringbuffer to write to
 *  \param value Character to write
 *  \return Number of characters written
 */
uint8_t ring_write(ringbuffer_t *ring, char value);

/** \brief Write a character to the ringbuffer (unsafe version)
 *
 *  This does not perform any interrupt disabling.
 *
 *  Same as ring_write()
 */
uint8_t ring_write_unsafe(ringbuffer_t *ring, char value);

/** \brief Read a character from the ringbuffer
 *
 *  You should check it's readable with ring_readable() first or you will get odd results.
 *
 *  Note: this disables global interrupts. You may wish to use the unsafe
 *  versions instead, and selectively disable interrupts.
 *
 *  \param ring Ringbuffer to read
 *  \return Character from the ringbuffer (could be 0)
 */
char ring_read(ringbuffer_t *ring);

/** \brief Read a character from the ringbuffer (unsafe version)
 *
 *  This does not perform any interrupt disabling.
 *
 *  Same as ring_read()
 */
char ring_read_unsafe(ringbuffer_t *ring);

/** \brief Check to see if we have anything to read
 *
 *  Note: this disables global interrupts. You may wish to use the unsafe
 *  version instead, and selectively disable interrupts.
 *
 *  \param ring The ringbuffer to check
 *  \return 1 for characters still to read, 0 otherwise
 */
uint8_t ring_readable(ringbuffer_t *ring);

/** \brief Check to see if we have anything to read (unsafe version)
 *
 *  This does not perform any interrupt disabling.
 *
 *  Same as ring_readable()
 */
uint8_t ring_readable_unsafe(ringbuffer_t *ring);

#ifdef __cplusplus
}
#endif

#endif // RINGBUFFER_H_INCLUDED
