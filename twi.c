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
	TWI_t *hw; /* pointer to real hardware */
	twi_rwmode_t rw; /* what mode we were last asked to start for */
	uint16_t timeout_us; /* How many us to wait before giving up */
} twi_port_t;

twi_port_t *twi_ports[MAX_TWI_PORTS] = TWI_INIT_PORTS;

int twi_wait_busowner(TWI_t *hw, uint16_t t);
int twi_wait_rwif(TWI_t *hw, uint16_t t);

/* initalise a port */
int twi_init(twi_portname_t port, uint16_t speed, uint16_t timeout_us) {
	uint32_t baud;

	if (twi_ports[port] || port >= MAX_TWI_PORTS) {
        k_err("no such port %d",port);
		return -ENODEV;
	}

	twi_ports[port] = malloc(sizeof(twi_port_t));

    /* reset the mode */
    twi_ports[port]->rw = twi_mode_read; /* well, it'll do */
    twi_ports[port]->timeout_us = timeout_us;

	switch (port) {
#if defined(TWIC)
		case twi_c:
			twi_ports[port]->hw = &TWIC;
			break;
#endif
#if defined(TWID)
		case twi_d:
			twi_ports[port]->hw = &TWID;
			break;
#endif
#if defined(TWIE)
		case twi_e:
			twi_ports[port]->hw = &TWIE;
			break;
#endif
#if defined(TWIF)
		case twi_f:
			twi_ports[port]->hw = &TWIF;
			break;
#endif
	}

	/* calculate the TWI baud rate for the CPU frequency */
	baud = ((uint32_t)F_CPU / (2000UL*(uint32_t)speed))-5;

	k_debug("port %d: baud=%d, timeout=%d",port,(uint8_t)baud,timeout_us);

	twi_ports[port]->hw->MASTER.BAUD = (uint8_t) baud;

	/* enable it */
	twi_ports[port]->hw->MASTER.CTRLA |= TWI_MASTER_ENABLE_bm;
#ifdef TWI_USE_TIMEOUT
	twi_ports[port]->hw->MASTER.CTRLB |= TWI_MASTER_TIMEOUT_200US_gc;
#endif
	/* force into idle */
	twi_ports[port]->hw->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;

	return 0;
}

int twi_wait_busowner(TWI_t *hw, uint16_t t) {
    while ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_IDLE_gc &&
            (hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_OWNER_gc) {
        t--;
        if (t == 0) {
            return -ETIME;
        }
        _delay_us(1);
    }
    return 0;
}

int twi_wait_rwif(TWI_t *hw, uint16_t t) {
    while ((hw->MASTER.STATUS & (TWI_MASTER_WIF_bm | TWI_MASTER_RIF_bm)) == 0) {
        t--;
        if (t == 0) {
            return -ETIME;
        }
        _delay_us(1);
    }
    return 0;
}

/* issue (repeated-)start for TWI master mode */
/* repeated start is just a side effect of calling this when already owning the bus */
int twi_start(twi_portname_t port, uint8_t addr, twi_rwmode_t rw) {
    TWI_t *hw; /* save us typing the array out all the time */

    /* check we're operating on a real port */
    if (!twi_ports[port] || port >= MAX_TWI_PORTS) {
        k_err("no such port %d",port);
        return -ENODEV;
    }

    if (rw > twi_mode_read) {
        k_err("invalid mode");
        return -EINVAL;
    }

    hw = twi_ports[port]->hw;

    /* start by acquiring the bus */

    k_debug("wait for bus acquire",twi_ports[port]->timeout_us);

    if (twi_wait_busowner(hw,twi_ports[port]->timeout_us)) {
        k_err("timeout for bus acquire");
        return -ETIME;
    }

    k_debug("hailing %02x",addr);
    if (rw == twi_mode_read) {
        hw->MASTER.ADDR = (addr << 1) | 0x1;
    } else {
        hw->MASTER.ADDR = (addr << 1);
    }

    if (twi_wait_rwif(hw,twi_ports[port]->timeout_us)) {
        k_warn("no response at addr %02x",addr);
        return -ENODEV;
    }

    k_debug("wif/rif set");

    /* at this point, we need to check what the status is */
    if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
        k_warn("lost bus during start");
        /* we failed to acquire the bus for some reason, toss error */
        return -EBUSY;
    }

    /* we got a NAK, then toss error. Note: this probably never gets hit */
    if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
        k_warn("no response at addr %02x",addr);
        /* issue a stop to release the bus */
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    /* these cases are different because we should have gotten a byte
     * from the device already, in which case RIF is set too */
    switch (rw) {
        case twi_mode_write:
            if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0) {
                k_debug("device %02x responded",addr);
                twi_ports[port]->rw = twi_mode_write;
                return 0;
            }
            break;
        case twi_mode_read:
            if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0 &&
                    hw->MASTER.STATUS & TWI_MASTER_RIF_bm) {
                k_debug("device %02x responded",addr);
                twi_ports[port]->rw = twi_mode_read;
                return 0;
            }
            break;
        default:
            break;
    }

    k_err("protocol error, giving up");
    hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    return -EIO;
}

int twi_write(twi_portname_t port, void *buf, uint16_t len, twi_end_t endstate) {
    TWI_t *hw; /* the HW pointer */

    if (!twi_ports[port] || port >= MAX_TWI_PORTS) {
        k_err("no such port %d",port);
		return -ENODEV;
	}

    if (twi_ports[port]->rw != twi_mode_write) {
        k_err("write called in read mode");
        return -EINVAL;
    }

    hw = twi_ports[port]->hw;

    /* at this point, we need to check what the status is */
    if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_OWNER_gc) {
        k_err("lost bus before write started");
        return -EIO;
    }

    /* write the data out to the endpoint */
    while (len) {
        /* write a byte of data to the bus, and wait for interrupt */
        k_debug("out: %02x",*(uint8_t *)buf);
        hw->MASTER.DATA = *(uint8_t *)buf;

        buf++;
        len--;

        k_debug("wait for write complete");
        if (twi_wait_rwif(hw,twi_ports[port]->timeout_us)) {
            k_err("write timeout, releasing bus");
            /* return the bus to others, regardless */
            hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            return -EIO;
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* give up, something else is trashing the bus */
            k_err("bus became busy during write, giving up");
            return -EIO;
        }

        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
            /* we got a NAK, so in either case, we stop sending stuff */
            k_debug("nak recieved");
            /* if we were expecting to release bus, do so. */
            /* we force release when bytes pending */
            if (endstate == twi_stop || len) {
                k_debug("releasing bus");
                hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            }
            if (len) {
                /* we had bytes left to send */
                k_err("slave NAK while bytes pending");
                return -EIO;
            }
            return 0; /* we had nothing left to send anyway, so we're fine */
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0) {
            /* all good */
            k_debug("ack received");
            continue;
        }

        k_err("protocol error, giving up");
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    k_debug("tx complete");

    if (endstate == twi_stop) {
        /* release the bus */
        k_debug("releasing bus");
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    }

    return 0;
}

int twi_read(twi_portname_t port, void *buf, uint16_t len, twi_end_t endstate) {
    TWI_t *hw; /* the HW pointer */

    if (!twi_ports[port] || port >= MAX_TWI_PORTS) {
        k_err("no such port %d",port);
		return -ENODEV;
	}

    if (twi_ports[port]->rw != twi_mode_read) {
        k_err("read called in write mode");
        return -EINVAL;
    }

    hw = twi_ports[port]->hw;

    /* at this point, we need to check what the status is */
    if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_OWNER_gc) {
        k_err("lost bus before write started");
        return -EIO;
    }

    /* read the data as it comes in */

    while (len) {
        k_debug("waiting for next rx");

        /* this will have RIF set on the first pass from twi_start() in read
         * mode so should just immediately RX. Check WIF for bus errors */
        if (twi_wait_rwif(hw,twi_ports[port]->timeout_us)) {
            k_err("read timeout, releasing bus");
            /* always release the bus */
            hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            return -ETIME;
        }

        /* check for bus error */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* give up, something else is trashing the bus */
            k_err("bus became busy during read, giving up");
            return -EIO;
        }

        /* do actual RX */
        *(uint8_t *)buf = hw->MASTER.DATA;
        k_debug("rx %02x",*(uint8_t *)buf);
        buf++;
        len--;

        if (!len && endstate == twi_stop) {
            k_debug("rx complete, send nak + releasing bus");
            hw->MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
            break;
        }

        /* we may not be expecting any more reads at this point, but
         * send the ack for now and well.. we'll just toss the byte */
        k_debug("expect more, send ack");
        hw->MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;

    }

    /* all done */
    return 0;
}
