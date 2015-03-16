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
 *  + spi_txrx(): write/read from an SPI device arbitrary lengths
 *
 *  Note: all handling of CS lines must be done in your own code,
 *  this code does not presume any specific CS state.
 *
 *  Note: you MUST set the port's slave-select pin to OUTPUT if you
 *  intend to use this port in master mode, EVEN IF you don't have
 *  a CS line attached to it.
 *
 *  To Do:
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
	PORT_t *port; /**< Pointer to the port to use */
	uint8_t txdummy; /**< What to pad generated TX with */
	/* Fixme: add ringbuffers and callback hooks */
} spi_port_t;

spi_port_t *spi_ports[MAX_SPI_PORTS] = SPI_PORT_INIT; /**< SPI port abstractions */

/* Iniitalise a port */
int spi_init(spi_portname_t portnum) {

	if (spi_ports[portnum] || portnum >= MAX_SPI_PORTS) {
		/* refuse to re-initalise a port or one not allocatable */
		return -ENODEV;
	}

	/* create the metadata for the port */
	spi_ports[portnum] = malloc(sizeof(spi_port_t));
	if (!spi_ports[portnum]) {
		return -ENOMEM;
	}

    /* populate HW structs */
    switch (portnum) {
#if defined(SPIC)
		case spi_c:
			PR.PRPC &= ~(PR_SPI_bm); /* ensure it's powered up */
			spi_ports[portnum]->port = &PORTC;
			spi_ports[portnum]->hw = &SPIC; /* associate HW */
			break;
#endif
#if defined(SPID)
		case spi_d:
			PR.PRPD &= ~(PR_SPI_bm); /* ensure it's powered up */
			spi_ports[portnum]->port = &PORTD;
			spi_ports[portnum]->hw = &SPID; /* associate HW */
			break;
#endif
#if defined(SPIE)
		case spi_e:
			PR.PRPE &= ~(PR_SPI_bm); /* ensure it's powered up */
			spi_ports[portnum]->port = &PORTE;
			spi_ports[portnum]->hw = &SPIE; /* associate HW */
			break;
#endif
#if defined(SPIF)
		case spi_f:
			PR.PRPF &= ~(PR_SPI_bm); /* ensure it's powered up */
			spi_ports[portnum]->port = &PORTF;
			spi_ports[portnum]->hw = &SPIF; /* associate HW */
			break;
#endif
    }

    /* configure pins */
    spi_ports[portnum]->port->DIRSET = (PIN7_bm | PIN5_bm | PIN4_bm); /* make outputs */
	spi_ports[portnum]->port->DIRCLR = (PIN6_bm); /* make inputs */
	/* enable a pull-up on the MISO pin */
	//spi_ports[portnum]->port->PIN6CTRL |= PORT_OPC_PULLUP_gc;

    /* enable the port and we're good to go */
    spi_ports[portnum]->hw->CTRL = (SPI_ENABLE_bm | SPI_MASTER_bm);
    //SPID.CTRL = (SPI_ENABLE_bm | SPI_MASTER_bm);

    return 0;
}

/* set configuration for a given port */
int spi_conf(spi_portname_t portnum, spi_clkdiv_t clock, spi_mode_t mode, uint8_t txdummy) {
	if (portnum >= MAX_SPI_PORTS || !spi_ports[portnum]) {
		return -ENODEV;
	}

	if (mode > 0x3 || clock > 0x6) {
		return -EINVAL;
	}

	/* apply the config to the port */
	spi_ports[portnum]->txdummy = txdummy;

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
int spi_txrx(spi_portname_t portnum, void *tx_buf, void *rx_buf, uint16_t len) {
    /* this is marked as unused explicitly as a discard */
	uint8_t __attribute__((unused)) discard;

	/* check we have a sane port first */
	if (portnum >= MAX_SPI_PORTS || !spi_ports[portnum]) {
		return -ENODEV;
	}

    /* since we have three cases, depending on NULLs, check for NULL buffers
     * at the outset. Less branching this way. */
    if (tx_buf && rx_buf) {
        /* we have both buffers, read from tx, write to rx */
        while (len--) {
            spi_ports[portnum]->hw->DATA = *(uint8_t *)tx_buf;
            tx_buf++;
            /* wait for complete */
            while (!(spi_ports[portnum]->hw->STATUS & SPI_IF_bm));
            /* NULL BODY */
            /* read the byte into the rx buffer */
            *(uint8_t *)rx_buf = spi_ports[portnum]->hw->DATA;
            rx_buf++;
        }
        return 0;
    }

    if (tx_buf && !rx_buf) {
        /* discard RX, read from tx */
        while (len--) {
            spi_ports[portnum]->hw->DATA = *(uint8_t *)tx_buf;
            tx_buf++;
            while (!(spi_ports[portnum]->hw->STATUS & SPI_IF_bm));
            /* NULL BODY */
            /* we have to read this even tho we are discarding it */
            discard = spi_ports[portnum]->hw->DATA;
        }
        return 0;
    }

    if (rx_buf && !tx_buf) {
        /* generate zeros for TX, write to rx */
        while (len--) {
            spi_ports[portnum]->hw->DATA = spi_ports[portnum]->txdummy;
            while (!(spi_ports[portnum]->hw->STATUS & SPI_IF_bm));
            /* NULL BODY */
            /* read the byte into the rx buffer */
            *(uint8_t *)rx_buf = spi_ports[portnum]->hw->DATA;
            rx_buf++;
        }
        return 0;
    }

    /* if we reach this, we have something odd like NULL/NULL */
    /* write zeros and discard, this might be used for a drain? */
    while (len--) {
            spi_ports[portnum]->hw->DATA = spi_ports[portnum]->txdummy;
            while (!(spi_ports[portnum]->hw->STATUS & SPI_IF_bm));
            /* we have to read this even tho we are discarding it */
            discard = spi_ports[portnum]->hw->DATA;
    }

	return 0;
}

