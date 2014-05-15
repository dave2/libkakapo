/* USART port definitions and public interface */

/* Copyright (C) 2009-2014 David Zanetti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef USART_H_INCLUDED
#define USART_H_INCLUDED

#include "global.h"

/** \file
 *  \brief USART driver public API
 *
 *  Interrupt-driven USART driver with runtime baud rate calculation,
 *  flexible TX/RX buffering, and RX interrupt call hooks to faciliate
 *  integration in main loop processing. It creates FILE *stream
 *  pointers suitable for avr-libc <stdio.h> functions.
 *
 *  Process of using a port is:
 *
 *  + usart_init(); to prepare the internal structures
 *
 *  + usart_conf(); to configure baud, bits, parity etc.
 *
 *  + usart_map_stdio(); returns a FILE * suitable for stdio functions
 *
 *  + usart_run(); to get things going
 *
 *  Can be easily expanded to cover more ports than the 2 currently
 *  implemented, as all code is generic
 */

#define U_FEAT_NONE 0 /**< USART port feature: None */
#define U_FEAT_ECHO 1 /**< USART port feature: echoback inside driver */

/** \brief Enum for parity types */
typedef enum {
	none, /**< No parity */
	even, /**< Even parity */
	odd   /**< Odd parity */
} parity_t;

/* initalise serial port X */
/** \brief Initalise the given serial port with the sized buffers
 *  \param portnum Number of the port
 *  \param rx_size Size of the RX buffer (must be power of two)
 *  \param tx_size Size of the TX buffer (must be power of two)
 *  \return 0 for success, negative errors.h values otherwise
 */
int usart_init(uint8_t portnum, uint8_t rx_size, uint8_t tx_size);

/** \brief Set parameters for the port, speed and such like
 *
 *  This must be called when the port is suspended or very bad things
 *  will happen. It *can* be called on a port which has been associated
 *  with a stream.
 *
 *  \param portnum Number of the port
 *  \param baud Baudrate
 *  \param bits Bits per char (note: 9 is not supported)
 *  \param parity Parity mode (none, even, odd)
 *  \param stop Stop bits
 *  \param features Features (see U_FEAT_*)
 *  \param rxfn Pointer to function for RX interrupt hook, NULL for no
 *  hook. Must return void, accept uint8_t of current char.
 *  \return 0 for success, negative errors.h values otherwise
 */
int usart_conf(uint8_t portnum, uint32_t baud, uint8_t bits,
	parity_t parity, uint8_t stop, uint8_t features, void (*rxfn)(uint8_t));

/** \brief Start listening for events and characters, also allows
 *  TX to begin
 *  \param portnum Number of the port
 *  \param state boolean, true or false
 */
int usart_run(uint8_t portnum, bool_t state);

/** \brief Flush the buffers for the serial port
 *  \param portnum Number of the port
 *  \return 0 for success, negative errors.h values otherwise
 */
int usart_flush(uint8_t portnum);

/** \brief Associate a serial port with a stdio stream
 *
 *  This allows libc <stdio.h> functions to be used with
 *  the port. Similar to avr-libc behaviour, the first call to this will
 *  be nominated as stdout, stdin, and stderr. Subsequent calls will
 *  be unique streams.
 *
 *  \param portnum Number of the port
 *  \return FILE handle, NULL if allocated failed
 */
FILE *usart_map_stdio(uint8_t portnum);

#endif // usart_H_INCLUDED
