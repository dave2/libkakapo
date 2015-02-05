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

#undef DEBUG_TWI

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
		return -ENODEV;
	}

    hw = twi_ports[portnum]->hw;

    /* keep looping around until we get the bus or attempts max reached */
    while (n++ < 10) {
#ifdef DEBUG_TWI
        printf_P(PSTR("wait idle\r\n"));
#endif

        while ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_IDLE_gc);

#ifdef DEBUG_TWI
        printf_P(PSTR("write %02x\r\n"),addr);
#endif

        hw->MASTER.ADDR = (addr << 1);

        while ((hw->MASTER.STATUS & TWI_MASTER_WIF_bm) == 0);

#ifdef DEBUG_TWI
        printf_P(PSTR("WIF set\r\n"));
#endif

        /* at this point, we need to check what the status is */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* we failed to acquire the bus for some reason, try again */
#ifdef DEBUG_TWI
            printf_P(PSTR("i2c busy, trying again\r\n"));
#endif
            continue;
        }

        /* we got a NAK, then fail the whole transaction */
        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
#ifdef DEBUG_TWI
            printf_P(PSTR("no device ack'd, giving up\r\n"));
#endif
            /* issue a stop to release the bus */
            hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            return -EIO;
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0) {
#ifdef DEBUG_TWI
            printf_P(PSTR("dev ack (try %d)\r\n"),n);
#endif
            break;
        }

#ifdef DEBUG_TWI
        printf_P(PSTR("protocol error, giving up\r\n"));
#endif
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    /* now we should be able to write the data */
    while (len) {
        /* write a byte of data to the bus, and wait for interrupt */
#ifdef DEBUG_TWI
        printf_P(PSTR("out: %02x"),*(uint8_t *)buf);
#endif
        hw->MASTER.DATA = *(uint8_t *)buf;

        buf++;
        len--;

        while ((hw->MASTER.STATUS & TWI_MASTER_WIF_bm) == 0);

#ifdef DEBUG_TWI
        printf_P(PSTR("WIF set\r\n"));
#endif

        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* give up, something else is trashing the bus */
#ifdef DEBUG_TWI
            printf_P(PSTR("i2c became busy, giving up\r\n"));
#endif // DEBUG_TWI
            return -EIO;
        }

        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
            /* we got a NAK, so in either case, we stop sending stuff */
#ifdef DEBUG_TWI
            printf_P(PSTR("NAK recieved\r\n"));
#endif // DEBUG_TWI
            if (stop) {
#ifdef DEBUG_TWI
                printf_P(PSTR("issuing stop\r\n"));
#endif // DEBUG_TWI
                hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            }
            if (len) {
                /* we had bytes left to send */
                return -EIO;
            }
            return 0; /* we had nothing left to send anyway, so we're fine */
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0) {
            /* all good */
#ifdef DEBUG_TWI
            printf_P(PSTR("slaves wants more\r\n"));
#endif // DEBUG_TWI
            continue;
        }

#ifdef DEBUG_TWI
        printf_P(PSTR("protocol error, giving up\r\n"));
#endif
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

#ifdef DEBUG_TWI
    printf_P(PSTR("wrote everything\r\n"));
#endif // DEBUG_TWI
    if (stop) {
#ifdef DEBUG_TWI
        printf_P(PSTR("issuing stop\r\n"));
#endif // DEBUG_TWI
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
    }
    return 0;
}

int twi_read(twi_portname_t portnum, uint8_t addr, void *buf, uint8_t len, uint8_t stop) {
    uint8_t n = 0;
    TWI_t *hw; /* the HW pointer */

    if (!twi_ports[portnum] || portnum >= MAX_TWI_PORTS) {
		return -ENODEV;
	}

    hw = twi_ports[portnum]->hw;

    /* keep looping around until we get the bus or attempts max reached */
    while (n++ < 10) {
#ifdef DEBUG_TWI
        printf_P(PSTR("wait idle\r\n"));
#endif

        while ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_IDLE_gc &&
        (hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) != TWI_MASTER_BUSSTATE_OWNER_gc);
#ifdef DEBUG_TWI
        printf_P(PSTR("read %02x\r\n"),addr);
#endif

        hw->MASTER.ADDR = (addr << 1) | 0x1;

        while ((hw->MASTER.STATUS & (TWI_MASTER_WIF_bm | TWI_MASTER_RIF_bm)) == 0);
#ifdef DEBUG_TWI
        printf_P(PSTR("WIF or RIF set\r\n"));
#endif

        /* at this point, we need to check what the status is */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* we failed to acquire the bus for some reason, try again */
#ifdef DEBUG_TWI
            printf_P(PSTR("i2c busy, trying again\r\n"));
#endif
            continue;
        }

        /* we got a NAK, then fail the whole transaction */
        if (hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
#ifdef DEBUG_TWI
            printf_P(PSTR("no device ack'd, giving up\r\n"));
#endif
            /* issue a stop to release the bus */
            hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
            return -EIO;
        }

        if ((hw->MASTER.STATUS & TWI_MASTER_RXACK_bm) == 0 &&
            hw->MASTER.STATUS & TWI_MASTER_RIF_bm) {
#ifdef DEBUG_TWI
            printf_P(PSTR("dev ack, data present (try %d)\r\n"),n);
#endif
            break;
        }

#ifdef DEBUG_TWI
        printf_P(PSTR("protocol error, giving up\r\n"));
#endif
        hw->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
        return -EIO;
    }

    while (len) {
        *(uint8_t *)buf = hw->MASTER.DATA;
#ifdef DEBUG_TWI
        printf_P(PSTR("rx %02x\r\n"),*(uint8_t *)buf);
#endif
        buf++;
        len--;

        /* we now need to decide what to do */
        if ((hw->MASTER.STATUS & TWI_MASTER_BUSSTATE_gm) == TWI_MASTER_BUSSTATE_BUSY_gc) {
            /* give up, something else is trashing the bus */
#ifdef DEBUG_TWI
            printf_P(PSTR("i2c became busy, giving up\r\n"));
#endif
            return -EIO;
        }

        /* if we have data to continue to RX, please provide it */
        if (!len) {
#ifdef DEBUG_TWI
            printf_P(PSTR("we will term\r\n"));
#endif
            hw->MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_RECVTRANS_gc;
            break;
        }

#ifdef DEBUG_TWI
        printf_P(PSTR("we expect more\r\n"));
#endif
        hw->MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;

#ifdef DEBUG_TWI
        printf_P(PSTR("waiting for next packet\r\n"));
#endif
        /* now wait for the next RIF or timeout */
        while ((hw->MASTER.STATUS & TWI_MASTER_RIF_bm) == 0 &&
            (hw->MASTER.STATUS & TWI_MASTER_WIF_bm) == 0);
    }

#ifdef DEBUG_TWI
    printf_P(PSTR("read everything\r\n"));
#endif
    if (stop) {
#ifdef DEBUG_TWI
        printf_P(PSTR("issuing stop\r\n"));
#endif
        hw->MASTER.CTRLC |= TWI_MASTER_CMD_STOP_gc;
    }
    return 0;
}
