#ifndef TWI_H_INCLUDED
#define TWI_H_INCLUDED

/* Copyright (C) 2014 David Zanetti
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* XMEGA TWI driver
 *
 * TWI (aka I2C, SMBus) provides a low-speed shared serial bus with addressing
 * for nodes, multiple masters and simple connections.
 *
 * Current implementation only supports master mode and 7-bit addressing.
 */


/** \brief Initalise a TWI port
 *  \param portnum Number of the port
 *  \param speed Speed of the port, in kHz
 *  \return 0 on success, errors.h otherwise
 */
int twi_init(uint8_t portnum, uint16_t speed);

/** \brief Write a byte sequence to the specified address (master)
 *  \param portnum Number of the port
 *  \param addr Target address
 *  \param buf Buffer to read from
 *  \param len Length of bytes to write
 *  \return 0 on success, errors.h otherwise
 */
int twi_write(uint8_t portnum, uint8_t addr, void *buf, uint8_t len);

/** \brief Read a byte sequence from the specified address (master)
 *  \param portnum Number of the port
 *  \param addr Target address
 *  \param buf Buffer to write to
 *  \param len Length of bytes to read
 *  \return 0 on success, errors.h otherwise
 */
int twi_read(uint8_t portnum, uint8_t addr, void *buf, uint8_t len);

#endif // TWI_H_INCLUDED
