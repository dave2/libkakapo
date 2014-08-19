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

#ifndef SPI_H_INCLUDED
#define SPI_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  \brief SPI driver public API
 *
 *  Simple SPI interface. Does not buffer or get driven from
 *  interrupts, but probably should be.
 *
 *  Usage:
 *
 *  + spi_init(): used to create the SPI instance
 *
 *  + spi_conf(): set the bitrate, polarity
 *
 *  + spi_txrx(): submit a single byte to the SPI interface
 *
 *  Note: all handling of CS lines must be done in your own code,
 *  this code does not presume any specific CS state.
 *
 *  Note: you MUST set the port's slave-select pin to OUTPUT if you
 *  intend to use this port in master mode, EVEN IF you don't have
 *  a CS line attached to it.
 *
 *  To Do:
 *
 *  + Make interrupt driven with buffering
 *
 *  + Support slave mode
 */

/** \brief SPI mode types */
typedef enum {
	spi_mode0 = 0, /**< Leading = rising/sample, trailing = falling/setup */
	spi_mode1, /**< Leading = rising/setup, trailing = falling/sample */
	spi_mode2, /**< Leading = falling/sample, trailing = rising/setup */
	spi_mode3, /**< Leading = falling/setup, trailing = rising/sample */
} spi_mode_t;

/** \brief SPI clock division */
typedef enum {
	spi_perdiv4 = 0, /**< CLKper/4, CLK2X 0 */
	spi_perdiv16, /**< CLKper/16, CLK2X 0 */
	spi_perdiv64, /**< CLKper/64, CLK2X 0 */
	spi_perdiv128, /**< CLKper/128, CLK2X 0 */
	spi_perdiv2, /**< CLKper/2, CLK2X 1 */
	spi_perdiv8, /**< CLKper/8, CLK2X 1 */
	spi_perdiv32, /**< CLKper/32, CLK2X 1 */
	/**< Not defined: CLKper/64, CLK2X 1 */
} spi_clkdiv_t;

/** \brief Define what SPI hardware exists */
/* E5/B1/B3 has 1 SPI, A1/A1U has 4, everyone else has 2 */
#if defined(_xmega_type_e5)|| defined(_xmega_type_b1) || defined (_xmega_type_b3)
#define MAX_SPI_PORTS 1 /**< Maximum number of SPI ports supported */
#define SPI_PORT_INIT {0} /**< Array to init port struct array with */
typedef enum {
    spi_c = 0,  /**< SPI on PORTC, pins 4,5,6,7 */
} spi_portname_t;
#elif defined (_xmega_type_a1) || defined (_xmega_type_a1u)
#define MAX_SPI_PORTS 4 /**< Maximum number of SPI ports supported */
#define SPI_PORT_INIT {0,0,0,0} /**< Array to init port struct array with */
typedef enum {
    spi_c = 0,  /**< SPI on PORTC, pins 4,5,6,7 */
    spi_d,      /**< SPI on PORTD, pins 4,5,6,7 */
    spi_e,      /**< SPI on PORTE, pins 4,5,6,7 */
    spi_f,      /**< SPI on PORTF, pins 4,5,6,7 */
} spi_portname_t;
#else
#define MAX_SPI_PORTS 2 /**< Maximum number of SPI ports supported */
#define SPI_PORT_INIT {0,0} /**< Array to init port struct array with */
typedef enum {
    spi_c = 0,  /**< SPI on PORTC, pins 4,5,6,7 */
    spi_d,      /**< SPI on PORTD, pins 4,5,6,7 */
} spi_portname_t;
#endif

/** \brief Initalise an SPI port
 *  \param portnum Number of the port
 *  \return 0 for sucess, errors.h otherwise
 */
int spi_init(spi_portname_t port);

/** \brief Configure and SPI port
 *
 *  SPI is clocked from a division of the 1x perpherial clock. Note that
 *  the maximum clock is F_CPU/2. SPI modes affect whether SCK is high
 *  or low at the start of a burst, and when MOSI/MISO are sampled.
 *
 *  \param portnum Number of the port
 *  \param clock Clock division from system clock
 *  \param mode SPI mode to use
 *  \return 0 for success, errors.h otherwise
 */
int spi_conf(spi_portname_t port, spi_clkdiv_t clock, spi_mode_t mode);

/** \brief Transmit/Receive SPI data
 *
 *  In SPI, receive and transmit are done at the same time on different lines.
 *  The master set MOSI to the next bit to be sent to the slave, clocks SCK, and
 *  the slave sets MISO to the next bit to be sent to the master before SCK
 *  cycle completes, followed by the master reading MISO for the read bit.
 *
 *  This means a "read" from a slave device is for the master to clock SCK
 *  and read MISO. Conversely, a "write" to a slave device is for the master
 *  to clock SCK with bits already set on MOSI. The other line is effectively
 *  discarded.
 *
 *  This function provides two buffer pointers: one for TX bytes from the master,
 *  and one for RX bytes from the slave to be stored in. They MUST NOT be the same
 *  bufffer, as this seems to cause corruption. If a buffer is NULL the it is
 *  either generated as 0s (for TX) or discarded (for RX). Generated 0s are required
 *  because that is how the AVR8 SPI master knows to generate clocks.
 *
 *  A valid way to use this would be to set up a TX buffer containing a command
 *  and then enough extra bytes to cover the response to that command, with an RX
 *  buffer sized the same. This allows a single call to do command-response.
 *
 *  \param portnum Number of the port
 *  \param tx_buf A buffer containing len bytes to transmit. May be NULL, this
 *  forces SPI port to transmit zeros (useful for reads).
 *  \param rx_buf A buffer containing len bytes to be written to from receive.
 *  May be NULL, this discards any read data (useful for writes).
 *  \return data received or errors.h
 */
int spi_txrx(spi_portname_t portnum, uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // SPI_H_INCLUDED
