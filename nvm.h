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

#ifndef NVM_H_INCLUDED
#define NVM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 * \brief
 * This is not a complete driver for the NVM subsystem. It provides
 * only helper functions to the prodsig and usersig rows. These are
 * one flash page each, and contain calibration and unique id values
 * for prodsig, and arbitrary data for the usersig.
 *
 * The usersig survives a chip erase, so it is safe for special data
 * such as device identification.
 */

/** \brief
 * Read the 11-byte serial number. This is composed of 6 bytes of
 * lot number, a 1 byte wafer number, and 4 bytes of co-ordinates.
 *
 * \param buf[11] An 11 byte buffer to write the serial number into
 */
void nvm_serial(uint8_t *buf);

/** \brief
 * Read the 16-bit ADC calibration value
 *
 * \return the calibration value
 */
uint16_t nvm_adccal(void);

/** \brief
 * Read the 16-bit temp sensor calibration value
 *
 * \return the calibration value
 */
uint16_t nvm_tempcal(void);

/** \brief
 * Read len number of bytes offset into the usersig page. There is no
 * expected format for this so the buffer is just filled with the byte
 * range.
 *
 * Note: the size of the user page is dependant on the chip. This function
 * will limit reading to the page.
 *
 * \param *buf Buffer to write into
 * \param offset Offset into the usersig
 * \param len Length of bytes to read
 * \return Actual number of bytes read
 */
uint16_t nvm_usersig(uint8_t *buf, uint16_t offset, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // NVM_H_INCLUDED
