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

#ifndef DEBUG_TWI
#define DEBUG_TWI 0
#endif

#if (DEBUG_TWI > 0)
#define KAKAPO_DEBUG_LEVEL DEBUG_TWI
#define KAKAPO_DEBUG_CHANNEL stdout
#endif

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include "global.h"
#include <util/delay.h>
#include "errors.h"
#include "twi.h"
#include "usart.h"
#include "debug.h"

typedef struct {
	TWI_t *hw;
} twi_port_t;

twi_port_t *twi_ports[MAX_TWI_PORTS] = TWI_INIT_PORTS;

/* initalise a port */
int twi_init(twi_portname_t portnum, uint16_t speed) {
	uint32_t baud;

	if (twi_ports[portnum] || portnum >= MAX_TWI_PORTS) {
        k_err_P(PSTR("no such port %d"),portnum);
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

	k_debug_P(PSTR("baud is %d"),(uint8_t)baud);

	twi_ports[portnum]->hw->MASTER.BAUD = (uint8_t) baud;

	/* enable it */
	twi_ports[portnum]->hw->MASTER.CTRLA |= TWI_MASTER_ENABLE_bm;
#ifdef TWI_USE_TIMEOUT
	twi_ports[portnum]->hw->MASTER.CTRLB |= TWI_MASTER_TIMEOUT_200US_gc;
#endif
	/* force into idle */
	twi_ports[portnum]->hw->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;

	return 0;
}

int twi_write(twi_portname_t portnum, uint8_t addr, void *buf, uint8_t len, uint8_t stop) {
    uint8_t n = 0;
    TWI_t *hw; /* the HW pointer */

    if (!twi_ports[portnum] || portnum >= MAX_TWI_PORTS) {
        k_err_P(PSTR("no such port %d"),portnum)
		return -ENODEV;
	}

    hw = twi_ports[portnum]->hw;

    /* keep looping around until we get the bus or attempts max reached */
    while (n++ < 10) {
        k_debug_P(PSTR("waiting for bus idle"));
        while ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_IDLE_gc);
        /* null body */

        k_debug_P(PSTR("write to %02x"),addr);
        hw->MASTER.ADDR = (addr << 1);

        while ((hw->MASTER.STATUS & TWI_MASTER_WIF_bm) == 0);
        /* null body */

        k_debug_P(PSTR("wif set"));

        /* at this point, we need to check what the status is */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* we failed to acquire the bus for some reason, try again */
            k_info_P(PSTR("bus busy, trying again"));
            continue;
        }

        /* we got a NAK, then fail the whole transaction */
        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
            k_warn_P(PSTR("no device responded at %02x, giving up"),addr);
            /* issue a stop to release the bus */
            hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            return -EIO;
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0) {
            k_debug_P(PSTR("device %02x responded (try %02x)"),addr,n);
            break;
        }

        k_err_P(PSTR("protocol error, giving up"));
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    /* now we should be able to write the data */
    while (len) {
        /* write a byte of data to the bus, and wait for interrupt */
        k_debug_P(PSTR("out: %02x"),*(uint8_t *)buf);
        hw->MASTER.DATA = *(uint8_t *)buf;

        buf++;
        len--;

        while ((hw->MASTER.STATUS & TWI_MASTER_WIF_bm) == 0);
        /* null body */

        k_debug_P(PSTR("wif set"));

        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* give up, something else is trashing the bus */
            k_err_P(PSTR("bus became busy during write, giving up"));
            return -EIO;
        }

        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
            /* we got a NAK, so in either case, we stop sending stuff */
            k_debug_P(PSTR("nak recieved"));
            if (stop) {
                k_debug_P(PSTR("issuing stop"));
                hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            }
            if (len) {
                /* we had bytes left to send */
                k_err_P(PSTR("slave %02x NAK while bytes pending"));
                return -EIO;
            }
            return 0; /* we had nothing left to send anyway, so we're fine */
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0) {
            /* all good */
            k_debug_P(PSTR("slave wants more bytes"));
            continue;
        }

        k_err_P(PSTR("protocol error, giving up"));
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    k_debug_P(PSTR("tx complete"));
    if (stop) {
        k_debug_P(PSTR("issuing stop"));
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    }
    return 0;
}

int twi_read(twi_portname_t portnum, uint8_t addr, void *buf, uint8_t len, uint8_t stop) {
    uint8_t n = 0;
    TWI_t *hw; /* the HW pointer */

    if (!twi_ports[portnum] || portnum >= MAX_TWI_PORTS) {
        k_err_P(PSTR("no such port %d"),portnum)
		return -ENODEV;
	}

    hw = twi_ports[portnum]->hw;

    /* keep looping around until we get the bus or attempts max reached */
    while (n++ < 10) {
        k_debug_P(PSTR("waiting for bus idle"));

        while ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_IDLE_gc &&
        (hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_OWNER_gc);
        /* null body */

        k_debug_P(PSTR("read from %02x"),addr);
        hw->MASTER.ADDR = (addr << 1) | 0x1;

        while ((hw->MASTER.STATUS & (TWI_MASTER_WIF_bm | TWI_MASTER_RIF_bm)) == 0);
        /* null body */

        k_debug_P(PSTR("wif or rif set"));

        /* at this point, we need to check what the status is */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* we failed to acquire the bus for some reason, try again */
            k_debug_P(PSTR("bus busy, trying again"));
            continue;
        }

        /* we got a NAK, then fail the whole transaction */
        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
            k_warn_P(PSTR("no device responded at %02x, giving up"),addr);
            /* issue a stop to release the bus */
            hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            return -EIO;
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0 &&
            hw->MASTER.STATUS & TWI_MASTER_RIF_bm) {
            k_debug_P(PSTR("device %02x responded (try %02x)"),addr,n);
            break;
        }

        k_err_P(PSTR("protocol error, giving up"));
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    while (len) {
        *(uint8_t *)buf = hw->MASTER.DATA;
        k_debug_P(PSTR("rx %02x"),*(uint8_t *)buf);
        buf++;
        len--;

        /* we now need to decide what to do */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* give up, something else is trashing the bus */
            k_err_P(PSTR("bus became busy during read, giving up"));
            return -EIO;
        }

        /* if we have data to continue to RX, please provide it */
        if (!len) {
            k_debug_P(PSTR("read complete, sending nak"));
            hw->MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_RECVTRANS_gc;
            break;
        }

        k_debug_P(PSTR("expecting more bytes, sending ack"));
        hw->MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;

        k_debug_P(PSTR("waiting for next rx"));
        /* now wait for the next RIF or timeout */
        while ((hw->MASTER.STATUS & TWI_MASTER_RIF_bm) == 0 &&
            (hw->MASTER.STATUS & TWI_MASTER_WIF_bm) == 0);
        /* null body */
    }

    if (stop) {
        k_debug_P(PSTR("issuing stop"));
        hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
    }
    return 0;
}
