/* USART driver */

/* Copyright (C) 2009-2014 David Zanetti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdlib.h>

#include "global.h"
#include <util/delay.h>

#include "usart.h"
#include "ringbuffer.h"
#include "errors.h"

/** \file
 *  \brief USART driver implementation
 *
 *  Interrupt-driven USART driver with runtime baud rate calculation,
 *  flexible TX/RX buffering, and RX interrupt call hooks to faciliate
 *  integration in main loop processing. It creates FILE *stream
 *  pointers suitable for avr-libc <stdio.h> functions.
 *
 *  Process of using a port is:
 *
 *  + usart_init(); to prepare the internal structures
 *
 *  + usart_conf(); to configure baud, bits, parity etc.
 *
 *  + usart_map_stdio(); returns a FILE * suitable for stdio functions
 *
 *  + usart_run(); to get things going
 *
 *  Can be easily expanded to cover more ports than the 2 currently
 *  implemented, as all code is generic
 */

/* hw access, buffers, and metadata about a usart port */

/** \struct usart_port_t
 *  \brief  Contains the abstraction of a hardware port
 */
typedef struct {
	USART_t *hw; /**< USART hardware IO registers */
	ringbuffer_t *txring; /**< TX ringbuffer */
	ringbuffer_t *rxring; /**< RX ringbuffer */
	uint8_t isr_level; /**< Level to run/restore interrupts at */
	uint8_t features; /**< Capabilities of the port, see U_FEAT_ */
	void (*rx_fn)(uint8_t); /**< Callback function for RX */
} usart_port_t;

#define USART_RX_PULLUP /**< Should we force RX pin to have input pull-up */

usart_port_t *ports[MAX_PORTS] = USART_PORT_INIT; /**< USART port abstractions */

/* private function prototypes */

/* Low-level ISR handlers, generic on which port they apply to */

/** \brief Handle a TX interrupt for the given port
 * \param port Port abstraction this event applies to
 *
 */
void _usart_tx_isr(usart_port_t *port);

/** \brief Handle an RX interrupt for the given port
 *  \param port Port abstraction this event applies to
 */
void _usart_rx_isr(usart_port_t *port);

/** \brief Start TX processing on the given port
 *  \param port Port abstraction this event applies to
 */
void _usart_tx_run(usart_port_t *port);

/** \brief Put hook for stdio functions.
 *
 *  See avr-libc stdio.h documentation
 */
int usart_put(char s, FILE *handle);

/** \brief Get hook for stdio functions.
 *
 *  See avr-libc stdio.h documentation
 */
int usart_get(FILE *handle);

/* Interrupt hooks and handlers */

/* handle a TX event */
/* since this fires on empty, it's safe to fire more than we actually need to */
void _usart_tx_isr(usart_port_t *port) {
	if (!port) {
		return; /* don't try to use uninitalised ports */
	}
	/* check to see if we have anything to send */
	if (!ring_readable_unsafe(port->txring)) {
		/* disable the interrupt and then exit, nothing more to do */
		port->hw->CTRLA = port->hw->CTRLA & ~(USART_DREINTLVL_gm);
		return;
	}
	/* TX the waiting packet */
	port->hw->DATA = ring_read_unsafe(port->txring);
}

/* handle an RX event */
void _usart_rx_isr(usart_port_t *port) {
	char s;

	if (!port) {
		return; /* don't try to use uninitalised ports */
	}

	s = port->hw->DATA; /* read the char from the port */
	ring_write_unsafe(port->rxring, s); /* if this fails we have nothing useful we can do anyway */

	if (port->features & U_FEAT_ECHO) {
		/* fixme: does this introduce another source of ring corruption? */
		ring_write_unsafe(port->txring,s);
		_usart_tx_run(port);
	}

	/* invoke the callback if one exists */
	if (port->rx_fn) {
		(*port->rx_fn)(s);
	}
}

/* make the given port start TXing */
void _usart_tx_run(usart_port_t *port) {
	/* enable interrupts for the appropriate port */
	port->hw->CTRLA = port->hw->CTRLA | (port->isr_level & USART_DREINTLVL_gm);
	return;
}

/* interrupt handlers */
#if defined(USARTC0)
ISR(USARTC0_DRE_vect) {
	_usart_tx_isr(ports[usart_c0]);
	return;
}
ISR(USARTC0_RXC_vect) {
	_usart_rx_isr(ports[usart_c0]);
	return;
}
#endif //defined(USARTC0)

#if defined(USARTD0)
ISR(USARTD0_DRE_vect) {
	_usart_tx_isr(ports[usart_d0]);
	return;
}

ISR(USARTD0_RXC_vect) {
	_usart_rx_isr(ports[usart_d0]);
	return;
}
#endif //defined(USARTD0)

#if defined(USARTC1)
ISR(USARTC1_DRE_vect) {
	_usart_tx_isr(ports[usart_c1]);
	return;
}
ISR(USARTC1_RXC_vect) {
	_usart_rx_isr(ports[usart_c1]);
	return;
}
#endif //defined(USARTC1)

#if defined(USARTD1)
ISR(USARTD1_DRE_vect) {
	_usart_tx_isr(ports[usart_d1]);
	return;
}

ISR(USARTD1_RXC_vect) {
	_usart_rx_isr(ports[usart_d1]);
	return;
}
#endif //defined(USARTD1)

#if defined(USARTE0)
ISR(USARTE0_DRE_vect) {
	_usart_tx_isr(ports[usart_e0]);
	return;
}

ISR(USARTE0_RXC_vect) {
	_usart_rx_isr(ports[usart_e0]);
	return;
}
#endif //defined(USARTE0)

#if defined(USARTE1)
ISR(USARTE0_DRE_vect) {
	_usart_tx_isr(ports[usart_e1]);
	return;
}

ISR(USARTE0_RXC_vect) {
	_usart_rx_isr(ports[usart_e1]);
	return;
}
#endif //defined(USARTE0)

#if defined(USARTF0)
ISR(USARTE0_DRE_vect) {
	_usart_tx_isr(ports[usart_f0]);
	return;
}

ISR(USARTE0_RXC_vect) {
	_usart_rx_isr(ports[usart_f0]);
	return;
}
#endif //defined(USARTE0)

#if defined(USARTF1)
ISR(USARTE0_DRE_vect) {
	_usart_tx_isr(ports[usart_f1]);
	return;
}

ISR(USARTE0_RXC_vect) {
	_usart_rx_isr(ports[usart_f1]);
	return;
}
#endif //defined(USARTE0)


/* initalise the structures and hardware */
int usart_init(usart_portname_t portnum, uint8_t rx_size, uint8_t tx_size) {

	if (ports[portnum] || portnum >= MAX_PORTS) {
		/* refuse to re-initalise a port or one not allocatable */
		return -ENODEV;
	}

	/* create the metadata for the port */
	ports[portnum] = malloc(sizeof(usart_port_t));
	if (!ports[portnum]) {
		return -ENOMEM;
	}

	/* create two ringbuffers, one for TX and one for RX */

	ports[portnum]->rxring = ring_create(rx_size-1);
	if (!ports[portnum]->rxring) {
		free(ports[portnum]);
		ports[portnum] = NULL;
		return -ENOMEM; /* FIXME: flag usart IO no longer works */
	}

	ports[portnum]->txring = ring_create(tx_size-1);
	if (!ports[portnum]->txring) {
		ring_destroy(ports[portnum]->rxring); /* since the first one succeeded */
		free(ports[portnum]);
		ports[portnum] = NULL;
		return -ENOMEM; /* FIXME: flag usart IO no longer works */
	}

	/* connect the hardware */
	switch (portnum) {
#if defined(USARTC0)
		case usart_c0:
			PR.PRPC &= ~(PR_USART0_bm);
			PORTC.DIRSET = PIN3_bm;
			PORTC.DIRCLR = PIN2_bm;
#ifdef USART_RX_PULLUP
            PORTC.PIN2CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTC0;
			break;
#endif // defined(USARTC0)
#if defined(USARTD0)
		case usart_d0:
			PR.PRPD &= ~(PR_USART0_bm);
			PORTD.DIRSET = PIN3_bm;
			PORTD.DIRCLR = PIN2_bm;
#ifdef USART_RX_PULLUP
            PORTD.PIN2CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTD0;
			break;
#endif // defined(USARTD0)
#if defined(USARTC1)
		case usart_c1:
			PR.PRPC &= ~(PR_USART1_bm);
			PORTC.DIRSET = PIN7_bm;
			PORTC.DIRCLR = PIN6_bm;
#ifdef USART_RX_PULLUP
            PORTC.PIN6CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTC1;
			break;
#endif // defined(USARTC1)
#if defined(USARTD1)
		case usart_d1:
			PR.PRPD &= ~(PR_USART1_bm);
			PORTD.DIRSET = PIN7_bm;
			PORTD.DIRCLR = PIN6_bm;
#ifdef USART_RX_PULLUP
            PORTD.PIN6CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTD1;
			break;
#endif // defined(USARTD1)
#if defined(USARTE0)
		case usart_e0:
			PR.PRPE &= ~(PR_USART0_bm);
			PORTE.DIRSET = PIN3_bm;
			PORTE.DIRCLR = PIN2_bm;
#ifdef USART_RX_PULLUP
            PORTE.PIN2CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTE0;
			break;
#endif // defined(USARTC0)
#if defined(USARTE1)
		case usart_e1:
			PR.PRPE &= ~(PR_USART1_bm);
			PORTE.DIRSET = PIN7_bm;
			PORTE.DIRCLR = PIN6_bm;
#ifdef USART_RX_PULLUP
            PORTE.PIN6CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTE1;
			break;
#endif // defined(USARTC0)
#if defined(USARTF0)
		case usart_f0:
			PR.PRPF &= ~(PR_USART0_bm);
			PORTF.DIRSET = PIN3_bm;
			PORTF.DIRCLR = PIN2_bm;
#ifdef USART_RX_PULLUP
            PORTF.PIN2CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTF0;
			break;
#endif // defined(USARTC0)
#if defined(USARTF1)
		case usart_f1:
			PR.PRPF &= ~(PR_USART1_bm);
			PORTF.DIRSET = PIN7_bm;
			PORTF.DIRCLR = PIN6_bm;
#ifdef USART_RX_PULLUP
            PORTF.PIN6CTRL |= PORT_OPC_PULLUP_gc;
#endif // USART_RX_PULLUP
			ports[portnum]->hw = &USARTF1;
			break;
#endif // defined(USARTC0)
	}

	/* fixme: allow seperate interrupt levels for different ports */
	ports[portnum]->isr_level = USART_DREINTLVL_LO_gc | USART_RXCINTLVL_LO_gc; /* low prio interrupt */

	/* default callback is NULL */
	ports[portnum]->rx_fn = NULL;

	/* port has no features by default */
	ports[portnum]->features = 0;

	/* enable rx interrupts */
	ports[portnum]->hw->CTRLA = (ports[portnum]->hw->CTRLA & ~(USART_RXCINTLVL_gm)) | (ports[portnum]->isr_level & USART_RXCINTLVL_gm);

	/* make sure low-level interrupts are enabled. Note: you still need to enable global interrupts */
	PMIC.CTRL |= PMIC_LOLVLEX_bm;

	return 0;
}

int usart_conf(usart_portname_t portnum, uint32_t baud, uint8_t bits,
	parity_t parity, uint8_t stop, uint8_t features,
	void (*rx_fn)(uint8_t)) {

	uint8_t mode = 0;
	uint8_t bscale = 0;
	uint16_t bsel;

	if (portnum > MAX_PORTS || !ports[portnum]) {
		return -ENODEV;
	}

	switch (bits) {
		case 5:
		case 6:
		case 7:
		case 8:
			mode = (bits - 5); /* cheating! */
			break;
		case 9:
			/* technically unsupported but here is the code to set it */
			mode = 7; /* cheating! */
			break;
		default:
			return -EINVAL;
	}

	stop--; /* SBMODE is 0 for 1 bit, 1 for 2 bits */
	switch (stop) {
		case 0:
		case 1:
			mode |= (stop & 1) << 3; /* more cheating */
			break;
		default:
			return -EINVAL;
	}

	switch (parity) {
		case none:
			break;
		case even:
			mode |= (2 << 4);
			break;
		case odd:
			mode |= (3 << 4);
			break;
		default:
			return -EINVAL;
	}

	/* apply the cheating way we worked out the modes for the port */
	ports[portnum]->hw->CTRLC = mode;

	/* apply features */
	ports[portnum]->features = features;

	/* RX hook, safe provided RX is disabled */
	ports[portnum]->rx_fn = rx_fn;

    /* baud rates faster than this are impossible */
	if (baud > (F_CPU/8)) {
		return -EBAUD;
	}

    /* these are hardcoded values for known frequencies all require CLK2X */
    /* FIXME: add more clock frequencies */
    switch (baud) {
#if (F_CPU == 32000000)
        case 9600:
            bsel = 3325;
            bscale = -3;
            break;
        case 19200:
            bsel = 3317;
            bscale = -4;
            break;
        case 38400:
            bsel = 3301;
            bscale = -5;
            break;
        case 57600:
            bsel = 2109;
            bscale = -5;
            break;
        case 115200:
            bsel = 2158;
            bscale = -6;
            break;
        case 921600:
            bsel = 428; /* limited by bscale */
            bscale = -7;
            break;
#elif (F_CPU == 2000000)
        case 9600:
            bsel = 3205;
            bscale = -7
            break;
        case 19200:
            bsel = 1539;
            bscale = -7;
            break;
        case 38400:
            bsel = 705;
            bscale = -7;
            break;
        case 57600:
            bsel = 428;
            bscale = -7;
            break;
        case 115200:
            bsel = 150;
            bscale = -7;
            break;
#endif
        default:
            return -EBAUD;
            break;
    }

    /* apply the results of the calculation */
	ports[portnum]->hw->BAUDCTRLA = bsel & 0xff;
	ports[portnum]->hw->BAUDCTRLB = (bsel >> 8) | ((bscale & 0xf)<< 4);
	ports[portnum]->hw->CTRLB |= USART_CLK2X_bm;

	/* all good! */
	return 0;
}

int usart_stop(usart_portname_t portnum) {
	if (portnum >= MAX_PORTS || !ports[portnum]) {
		/* do nothing */
		return -ENODEV;
	}
	ports[portnum]->hw->CTRLB |= (USART_RXEN_bm | USART_TXEN_bm);
	return 0;
}

int usart_run(usart_portname_t portnum) {
	if (portnum >= MAX_PORTS || !ports[portnum]) {
		/* do nothing */
		return -ENODEV;
	}

	ports[portnum]->hw->CTRLB |= (USART_RXEN_bm | USART_TXEN_bm);
	return 0;
}

int usart_flush(usart_portname_t portnum) {

	if (portnum >= MAX_PORTS || !ports[portnum]) {
		return -ENODEV;
	}

	/* disable the TX/RX engines */
	ports[portnum]->hw->CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);

	/* protect this from interrupts */
	ports[portnum]->hw->CTRLA = (ports[portnum]->hw->CTRLA & ~(USART_RXCINTLVL_gm | USART_DREINTLVL_gm));

	ring_reset(ports[portnum]->txring);
	ring_reset(ports[portnum]->rxring);

	/* re-enable RX interrupts */
	ports[portnum]->hw->CTRLA = (ports[portnum]->hw->CTRLA & ~(USART_RXCINTLVL_gm)) | (ports[portnum]->isr_level & USART_RXCINTLVL_gm);
	/* for TX, since we just wiped the ring buffer, it has nothing to TX, so don't enable DRE */

	/* re-enable the actual ports */
	ports[portnum]->hw->CTRLB |= (USART_RXEN_bm | USART_TXEN_bm);

	return 0;
}

int usart_put(char s, FILE *handle) {
	usart_port_t *port;
	/* reteieve the pointer to the struct for our hardware */
	port = (usart_port_t *)fdev_get_udata(handle);
	if (!port) {
		return _FDEV_ERR; /* avr-libc doesn't describe this, but never mind */
	}
	ring_write(port->txring,s);
	_usart_tx_run(port);
	return 0;
}

int usart_get(FILE *handle) {
	usart_port_t *port;
	/* retireve the pointer to the struct for our hardware */
	port = (usart_port_t *)fdev_get_udata(handle);
	if (!port) {
		return _FDEV_ERR;
	}
	if (ring_readable(port->rxring)) {
		return ring_read(port->rxring);
	}
	return _FDEV_EOF;
}

FILE *usart_map_stdio(usart_portname_t portnum) {
	FILE *handle = NULL;

	/* two steps, create the device, and then associate our ports struct with
	 * the passed portnum */

	handle = fdevopen(&usart_put,&usart_get);
	if (!handle) {
		return NULL;
	}
	/* associate our port struct with it */
	fdev_set_udata(handle,(void *)ports[portnum]);
	return handle;
}
