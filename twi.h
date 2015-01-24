/* Copyright (C) 2014 David Zanetti
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

/* XMEGA TWI driver
 *
 * TWI (aka I2C, SMBus) provides a low-speed shared serial bus with addressing
 * for nodes, multiple masters and simple connections.
 *
 * Current implementation only supports master mode and 7-bit addressing.
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

typedef enum {
    twi_stop = 0,
    twi_nostop,
} twi_stopmode_t;

/** \brief Initalise a TWI port
 *  \param portnum Number of the port
 *  \param speed Speed of the port, in kHz
 *  \return 0 on success, errors.h otherwise
 */
int twi_init(twi_portname_t port, uint16_t speed);

/** \brief Write a byte sequence to the specified address (master)
 *  \param portnum Number of the port
 *  \param addr Target address
 *  \param buf Buffer to read from
 *  \param len Length of bytes to write
 *  \return 0 on success, errors.h otherwise
 */
int twi_write(twi_portname_t port, uint8_t addr, void *buf, uint8_t len);

/** \brief Write a byte sequence to the specified address (master), without
 *  issuing a stop at the end.
 *  \param portnum Number of the port
 *  \param addr Target address
 *  \param buf Buffer to read from
 *  \param len Length of bytes to write
 *  \return 0 on success, errors.h otherwise
 */
int twi_write_nostop(twi_portname_t port, uint8_t addr, void *buf, uint8_t len);

/** \brief Read a byte sequence from the specified address (master)
 *  \param portnum Number of the port
 *  \param addr Target address
 *  \param buf Buffer to write to
 *  \param len Length of bytes to read
 *  \return 0 on success, errors.h otherwise
 */
int twi_read(twi_portname_t port, uint8_t addr, void *buf, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif // TWI_H_INCLUDED
