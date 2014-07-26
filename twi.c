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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include "global.h"
#include "errors.h"
#include "twi.h"
#include "usart.h"

typedef struct {
	TWI_t *hw;
} twi_port_t;

twi_port_t *twi_ports[MAX_TWI_PORTS] = TWI_INIT_PORTS;

/* initalise a port */
int twi_init(twi_portname_t portnum, uint16_t speed) {
	uint32_t baud;

	if (twi_ports[portnum] || portnum >= MAX_TWI_PORTS) {
		return -ENODEV;
	}

	twi_ports[portnum] = malloc(sizeof(twi_port_t));

	switch (portnum) {
#if defined(TWIC)
		case twi_c:
			twi_ports[portnum]->hw = &TWIC;
			break;
#endif
#if defined(TWID)
		case twi_d:
			twi_ports[portnum]->hw = &TWID;
			break;
#endif
#if defined(TWIE)
		case twi_e:
			twi_ports[portnum]->hw = &TWIE;
			break;
#endif
#if defined(TWIF)
		case twi_f:
			twi_ports[portnum]->hw = &TWIF;
			break;
#endif
	}

	/* calculate the TWI baud rate for the CPU frequency */
	baud = ((uint32_t)F_CPU / (2000UL*(uint32_t)speed))-5;
#ifdef DEBUG_TWI
	printf_P(PSTR("twi: baud is %d\r\n"),(uint8_t)baud);
#endif // DEBUG_TWI
	twi_ports[portnum]->hw->MASTER.BAUD = (uint8_t) baud;

	/* enable it */
	twi_ports[portnum]->hw->MASTER.CTRLA |= TWI_MASTER_ENABLE_bm;
	/* force into idle */
	twi_ports[portnum]->hw->MASTER.STATUS |= TWI_MASTER_BUSSTATE_IDLE_gc;

	return 0;
}

/* write to a givern address a lump of data */
int twi_write(twi_portname_t portnum,uint8_t addr,void *buf,uint8_t len) {
	twi_port_t *theport;

	if (portnum >= MAX_TWI_PORTS || !twi_ports[portnum]) {
		return -ENODEV;
	}
	theport = twi_ports[portnum];

#ifdef DEBUG_TWI
	printf_P(PSTR("twi: -> [%x]"),addr);
#endif // DEBUG_TWI

	/* write the left-shifted address plus the write bit to hw */
	theport->hw->MASTER.ADDR = (addr << 1);

	/* poll for the completion of the address write and bus ownership */
	while (!(theport->hw->MASTER.STATUS & TWI_MASTER_WIF_bm));
	/* null body */

	/* any error state at this point, just toss and error upwards */
	if (theport->hw->MASTER.STATUS & (TWI_MASTER_ARBLOST_bm |
					 				  TWI_MASTER_BUSERR_bm |
									  TWI_MASTER_RXACK_bm)) {
#ifdef DEBUG_TWI
		printf_P(PSTR("twi: err %x\r\n"),theport->hw->MASTER.STATUS);
#endif
		theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
		return -EIO;
	}

	/* we should have the bus and be free to write data now */
	while (len) {
#ifdef DEBUG_TWI
	printf_P(PSTR(" %x"),*(uint8_t *)buf);
#endif // DEBUG_TWI

		theport->hw->MASTER.DATA = *(uint8_t *)buf;
		buf++;
		len--;
		/* wait for write complete */
		while (!(theport->hw->MASTER.STATUS & TWI_MASTER_WIF_bm));
		/* null body */

		/* check to see if we have any errors */
		if (theport->hw->MASTER.STATUS & (TWI_MASTER_ARBLOST_bm |
										  TWI_MASTER_BUSERR_bm |
										  TWI_MASTER_RXACK_bm)) {
#ifdef DEBUG_TWI
			printf_P(PSTR("twi: err %x\r\n"),theport->hw->MASTER.STATUS);
#endif
			theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
			return -EIO;
		}
	}

	/* and issue the stop */
	theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
#ifdef DEBUG_TWI
	printf_P(PSTR(" [d]\r\n"));
#endif // DEBUG_TWI
	return 0;
}

int twi_read(twi_portname_t portnum,uint8_t addr, void *buf, uint8_t len) {
	twi_port_t *theport;

	if (portnum >= MAX_TWI_PORTS || !twi_ports[portnum]) {
		return -ENODEV;
	}
	theport = twi_ports[portnum];

#ifdef DEBUG_TWI
	printf_P(PSTR("twi: [%x] ->"),addr);
#endif // DEBUG_TWI

	/* write the left-shifted address plus the read bit to hw */
	theport->hw->MASTER.ADDR = (addr << 1) | 0x1;

	/* poll for the completion of the address write and bus ownership */
	while (!(theport->hw->MASTER.STATUS & TWI_MASTER_RIF_bm));
	/* null body */

	/* any error state at this point, just toss and error upwards */
	if (theport->hw->MASTER.STATUS & (TWI_MASTER_ARBLOST_bm |
									  TWI_MASTER_BUSERR_bm |
									  TWI_MASTER_RXACK_bm)) {
#ifdef DEBUG_TWI
		printf_P(PSTR("twi: err %x\r\n"),theport->hw->MASTER.STATUS);
#endif
		theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
		return -EIO;
	}

	/* we should have the bus and be free to write data now */
	while (len) {
		/* wait for RX complete */
		while (!(theport->hw->MASTER.STATUS & TWI_MASTER_RIF_bm));
		/* null body */

		/* check to see if we have any errors */
		if (theport->hw->MASTER.STATUS & (TWI_MASTER_ARBLOST_bm |
										  TWI_MASTER_BUSERR_bm |
										  TWI_MASTER_RXACK_bm)) {
#ifdef DEBUG_TWI
			printf_P(PSTR("twi: err %x\r\n"),theport->hw->MASTER.STATUS);
#endif
			theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
			return -EIO;
		}

		*(uint8_t *)buf = theport->hw->MASTER.DATA;
#ifdef DEBUG_TWI
	printf_P(PSTR(" %x"),*(uint8_t *)buf);
#endif // DEBUG_TWI
		buf++;
		len--;

		/* ack/nack as required */
		if (!len) {
			theport->hw->MASTER.CTRLC |= TWI_MASTER_ACKACT_bm;
		}
		/* and execute the command */
		theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_RECVTRANS_gc;
	}
	/* execute stop */
	theport->hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
#ifdef DEBUG_TWI
	printf_P(PSTR(" [d]\r\n"));
#endif // DEBUG_TWI
	return 0;
}
