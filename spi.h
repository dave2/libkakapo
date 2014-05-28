#ifndef SPI_H_INCLUDED
#define SPI_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

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
 *  \param portnum Number of the port
 *  \param clock Clock division from system clock
 *  \param mode SPI mode to use
 *  \return 0 for success, errors.h otherwise
 */
int spi_conf(spi_portname_t port, spi_clkdiv_t clock, spi_mode_t mode);

/** \brief Transmit/Receive SPI data
 *  Note: this is blocking code
 *  \param portnum Number of the port
 *  \param c Data to transmit
 *  \return data received or errors.h
 */
int spi_txrx(spi_portname_t port, uint8_t c);

#ifdef __cplusplus
}
#endif

#endif // SPI_H_INCLUDED
