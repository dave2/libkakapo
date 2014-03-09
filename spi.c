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
 *  \brief SPI driver implementation
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

#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "global.h"
#include "errors.h"
#include "spi.h"

/** \struct spi_port_t
 *  \brief Struct for holding port details
 */
typedef struct {
	SPI_t *hw; /**< Pointer to real hardware */
	/* Fixme: add ringbuffers and callback hooks */
} spi_port_t;

#define MAX_PORTS 2 /**< Maximum number of SPI ports supported */

spi_port_t *spi_ports[MAX_PORTS] = {0,0}; /**< SPI port abstractions */

/* Iniitalise a port */
int spi_init(uint8_t portnum) {

	if (spi_ports[portnum] || portnum >= MAX_PORTS) {
		/* refuse to re-initalise a port or one not allocatable */
		return -ENODEV;
	}

	/* create the metadata for the port */
	spi_ports[portnum] = malloc(sizeof(spi_port_t));
	if (!spi_ports[portnum]) {
		return -ENOMEM;
	}

    /* initilise the hardware */
    switch (portnum) {
		case 0:
			PR.PRPD &= ~(PR_SPI_bm); /* ensure it's powered up */
			PORTD.DIRSET = (PIN7_bm | PIN5_bm | PIN4_bm); /* make outputs */
			PORTD.DIRCLR = (PIN6_bm); /* make inputs */
			spi_ports[portnum]->hw = &SPID; /* associate HW */
			break;
		case 1:
			PR.PRPC &= ~(PR_SPI_bm); /* ensure it's powered up */
			PORTC.DIRSET = (PIN7_bm | PIN5_bm | PIN4_bm); /* make outputs */
			PORTC.DIRCLR = (PIN6_bm); /* make inputs */
			spi_ports[portnum]->hw = &SPIC; /* associate HW */
			break;
    }

    /* enable the port and we're good to go */
    spi_ports[portnum]->hw->CTRL = (SPI_ENABLE_bm | SPI_MASTER_bm);
    //SPID.CTRL = (SPI_ENABLE_bm | SPI_MASTER_bm);

    return 0;
}

/* set configuration for a given port */
int spi_conf(uint8_t portnum, spi_clkdiv_t clock, spi_mode_t mode) {
	if (portnum >= MAX_PORTS || !spi_ports[portnum]) {
		return -ENODEV;
	}

	if (mode > 0x3 || clock > 0x6) {
		return -EINVAL;
	}

	/* apply the config to the port */
	/* clear out bits we're touching */
	spi_ports[portnum]->hw->CTRL &= ~(SPI_MODE_gm | SPI_PRESCALER_gm |
									 SPI_CLK2X_bm);
	/* apply mode value */
	spi_ports[portnum]->hw->CTRL |= (mode << SPI_MODE_gp);
	/* apply lower bits of clock value */
	spi_ports[portnum]->hw->CTRL |= ((clock & 0x3) << SPI_PRESCALER_gp);
	/* higher clock settings have CLK2X set */
	spi_ports[portnum]->hw->CTRL |= (clock & 0x4 ? SPI_CLK2X_bm : 0);

	return 0;
}

/* handle a TX/RX, one char at a time
 * in SPI, there is no explicit separate TX and RX, instead in master the
 * RX is implied by TXing a 0x00 */
int spi_txrx(uint8_t portnum, uint8_t c) {
	uint8_t in;

	/* check we have a sane port first */
	if (portnum >= MAX_PORTS || !spi_ports[portnum]) {
		return -ENODEV;
	}

#ifdef DEBUG_SPI
	printf_P(PSTR("spi: tx %#x\r\n"),c);
#endif // DEBUG_SPI

	/* do it !*/
	spi_ports[portnum]->hw->DATA = c;

	while (!(spi_ports[portnum]->hw->STATUS & SPI_IF_bm));
	/* NULL BODY */

	in = spi_ports[portnum]->hw->DATA; /* side effect: clears IF bit */

#ifdef DEBUG_SPI
	printf_P(PSTR("spi: rx %#x\r\n"),in);
#endif // DEBUG_SPI

	return in;
}
