/* Copyright (C) 2014-2015 David Zanetti
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

#ifndef TWI_H_INCLUDED
#define TWI_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \brief XMEGA Two Wire Interface (TWI) driver
 *
 * TWI (aka I2C, SMBus) provides a low-speed shared serial bus with addressing
 * for nodes, multiple masters and simple connections.
 *
 * Current implementation only supports master mode and 7-bit addressing.
 *
 * This API mimics the TWI bus interactions normally provided in datasheets
 * for devices. A transaction is started with twi_start() to control the bus.
 * One or more writes/reads take place, and twi_stop() ends the transaction
 * and releases the bus.
 *
 * Changing from read to write or vice-versa requires either ending the
 * transaction, or a repeated-start by calling twi_start(). What is required
 * by any device is device-dependant.
 */

/** \brief An enum for each supported TWI interface on various families */
/* A1/A1U have 4 TWI interfaces, B1/B3/E5 has 1, everyone else has 2 */
#if defined(_xmega_type_A1U) || defined(_xmega_type_A1)

#define MAX_TWI_PORTS 4
#define TWI_INIT_PORTS {0,0,0,0}
typedef enum {
    twi_c = 0,
    twi_d,
    twi_e,
    twi_f,
} twi_portname_t;

#elif defined(_xmega_type_B1) || defined(_xmega_type_B3) || defined(_xmega_type_E5)

#define MAX_TWI_PORTS 1
#define TWI_INIT_PORTS {0}
typedef enum {
    twi_c = 0,
} twi_portname_t;

#else

#define MAX_TWI_PORTS 2
#define TWI_INIT_PORTS {0,0}
typedef enum {
    twi_c = 0,
    twi_e,
} twi_portname_t;

#endif

/** \brief TWI data direction indicators */
typedef enum {
    twi_mode_write = 0, /**< Action on device is write */
    twi_mode_read, /**< Action on device is read */
} twi_rwmode_t;

/** \brief Indication of what state to leave the bus */
typedef enum {
    twi_stop = 0, /**< Bus to be closed after activity */
    twi_more, /**< Bus to be left as-is at end of activity */
} twi_end_t;

/** \brief Initalise a TWI port (as master)
 *
 *  \param port Name of the TWI port to use
 *  \param speed Speed of the port, in kHz
 *  \param timeout_us Timeout of any function in us
 *  \return 0 on success, errors.h otherwise
 */
int twi_init(twi_portname_t port, uint16_t speed, uint16_t timeout_us);

/** \brief Start a TWI transaction (master)
 *
 *  Asserts a START on the TWI bus, and attempts to hail an address
 *  for read or write. If the previous activity wasn't a STOP, then
 *  this is a repeated-start condition.
 *
 *  \param port Name of the TWI port to use
 *  \param addr TWI address (7-bit)
 *  \param rw Indicate we are reading or writing device
 *  \return 0 on success, errors.h otherwise
 */
int twi_start(twi_portname_t port, uint8_t addr, twi_rwmode_t rw);

/** \brief Write a byte sequence to the specified address (master)
 *
 *  This can only be called after twi_start() in write mode has been
 *  called. It can be called multiple times, provided that endstate ==
 *  twi_more.
 *
 *  \param port Name of the TWI port to use
 *  \param addr Target address
 *  \param buf Buffer to read from
 *  \param len Length of bytes to write
 *  \param endstate What state to leave the bus on exit
 *  \return 0 on success, errors.h otherwise
 */
int twi_write(twi_portname_t port, void *buf, uint16_t len,
            twi_end_t endstate);

/** \brief Read a byte sequence from the specified address (master)
 *
 *  This can only be called after twi_start() in read mode has been
 *  called. It can be called multiple times, provided that endstate ==
 *  twi_more. Note: twi_stop implies NAK to the last byte received.
 *
 *  \param port Name of the TWI port to use
 *  \param addr Target address
 *  \param buf Buffer to write to
 *  \param len Length of bytes to read
 *  \param endstate What state to leave the bus on exit
 *  \return 0 on success, errors.h otherwise
 */
int twi_read(twi_portname_t port, void *buf, uint16_t len,
            twi_end_t endstate);


#ifdef __cplusplus
}
#endif

#endif // TWI_H_INCLUDED
