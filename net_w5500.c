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

/* Datasheet used:
 * http://wizwiki.net/wiki/lib/exe/fetch.php?media=products:w5500:w5500_ds_v102e_131114.pdf
 */

/* W5500 SPI comm structure
 * [addr1][addr0][blk/cmd][data][data]...
 * MSB transmission
 *
 * blk/cmd is broken up thus:
 * Block Address: 7-3
 * RWB: 2
 * Length: 1-0
 *
 * Length is 00 for variable length mode. we only implement variable */

#include <avr/io.h>
#include "global.h"
#include <stdio.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
#include "spi.h"
#include "net_w5500.h"
#include "errors.h"

/* W5500 Comms flags */
#define RWB 0x4 /**< Read/Write Bit, set is write */

/* W5500 block select (as needed for command block) */
#define BLK_COMMON 0x0 /**< Common block is 0 */
#define BLK_SOCKET_REG(n) (((n << 2)+1) << 3) /**< Socket N register block */
#define BLK_SOCKET_TX(n) (((n << 2)+2) << 3) /**< Socket N TX buffer block */
#define BLK_SOCKET_RX(n) (((n << 2)+3) << 3) /**< Socket N RX buffer block */

/* W5500 Register Map for common block */
#define COM_MR 0x00			/**< Mode */
#define COM_GAR0 0x01		/**< Gateway Octet 0 (MSB) */
#define COM_GAR1 0x02		/**< Gateway Octet 1 */
#define COM_GAR2 0x03		/**< Gateway Octet 2 */
#define COM_GAR3 0x04		/**< Gateway Octet 3 (LSB) */
#define COM_SUBR0 0x05		/**< Subnet Mask Octet 0 (MSB) */
#define COM_SUBR1 0x06		/**< Subnet Mask Octet 1 */
#define COM_SUBR2 0x07		/**< Subnet Mask Octet 2 */
#define COM_SUBR3 0x08		/**< Subnet Mask Octet 3 (LSB) */
#define COM_SHAR0 0x09		/**< Source HW MAC Octet 0 (MSB) */
#define COM_SHAR1 0x0a		/**< Source HW MAC Octet 1 */
#define COM_SHAR2 0x0b		/**< Source HW MAC Octet 2 */
#define COM_SHAR3 0x0c		/**< Source HW MAC Octet 3 */
#define COM_SHAR4 0x0d		/**< Source HW MAC Octet 4 */
#define COM_SHAR5 0x0e		/**< Source HW MAC Octet 5 (LSB) */
#define COM_SIPR0 0x0f		/**< Source IP Octet 0 (MSB) */
#define COM_SIPR1 0x10		/**< Source IP Octet 1 */
#define COM_SIRP2 0x11		/**< Source IP Octet 2 */
#define COM_SIRP3 0x12		/**< Source IP Octet 3 (LSB) */
#define COM_INTLEVEL0 0x13	/**< Interrupt Low Level Timer (MSB) */
#define COM_INTLEVEL1 0x14	/**< Interrupt Low Level Timer (LSB) */
#define COM_IR 0x15			/**< Interrupt */
#define COM_IMR 0x16		/**< Interrupt Mask */
#define COM_SIR 0x17		/**< Socket Interrupt */
#define COM_SIMR 0x18		/**< Socket Interrupt Mask */
#define COM_RTR0 0x19		/**< Retry Time (MSB) */
#define COM_RTR1 0x1a		/**< Retry Time (LSB) */
#define COM_RCR 0x1b		/**< Retry Count */
#define COM_PTIMER 0x1c		/**< PPP LCP Request Timer */
#define COM_PMAGIC 0x1d		/**< PPP LCP Magic Number */
#define COM_PHAR0 0x1e		/**< PPP Dest HW MAC Octet 0 (MSB) */
#define COM_PHAR1 0x1f		/**< PPP Dest HW MAC Octet 1 */
#define COM_PHAR2 0x20		/**< PPP Dest HW MAC Octet 2 */
#define COM_PHAR3 0x21		/**< PPP Dest HW MAC Octet 3 */
#define COM_PHAR4 0x22		/**< PPP Dest HW MAC Octet 4 */
#define COM_PHAR5 0x23		/**< PPP Dest HW MAC Octet 5 (LSB) */
#define COM_PSID0 0x24		/**< PPP Session ID (MSB) */
#define COM_PSID1 0x25		/**< PPP Session ID (LSB) */
#define COM_PMRU0 0x26		/**< PPP MSS (MSB) */
#define COM_PMRU1 0x27		/**< PPP MSS (LSB) */
#define COM_UIPR0 0x28		/**< Unreach IP Octet 0 (MSB) */
#define COM_UIPR1 0x29		/**< Unreach IP Octet 1 */
#define COM_UIPR2 0x2a		/**< Unreach IP Octet 2 */
#define COM_UIPR3 0x2b		/**< Unreach IP Octet 3 (LSB) */
#define COM_UPORTR0 0x2c	/**< Unreach Port (MSB) */
#define COM_UPORTR1 0x2d	/**< Unreach Port (LSB) */
#define COM_PHRCFGR 0x2e	/**< PHY Configuration */
/* 0x2f - 0x28 reserved */
#define COM_VERSIONR 0x39	/**< Chip Version */
/* 0x3a - 0xffff reserved */

/* W5500 Socket Register */
#define SOCK_MR 0x00		/**< Socket Mode */
#define SOCK_CR 0x01		/**< Socket Command */
#define SOCK_IR 0x02		/**< Socket Interrupt */
#define SOCK_SR 0x03		/**< Socket Status */
#define SOCK_PORT0 0x04		/**< Socket Source Port (MSB) */
#define SOCK_PORT1 0x05		/**< Socket Source Port (LSB) */
#define SOCK_DHAR0 0x06		/**< Socket Dest HW MAC Octet 0 (MSB) */
#define SOCK_DHAR1 0x07		/**< Socket Dest HW MAC Octet 1 */
#define SOCK_DHAR2 0x08		/**< Socket Dest HW MAC Octet 2 */
#define SOCK_DHAR3 0x09		/**< Socket Dest HW MAC Octet 3 */
#define SOCK_DHAR4 0x0a		/**< Socket Dest HW MAC Octet 4 */
#define SOCK_DHAR5 0x0b		/**< Socket Dest HW MAC Octet 5 (LSB) */
#define SOCK_DIPR0 0x0c		/**< Socket Dest IP Octet 0 (MSB) */
#define SOCK_DIPR1 0x0d		/**< Socket Dest IP Octet 1 */
#define SOCK_DIPR2 0x0e		/**< Socket Dest IP Octet 2 */
#define SOCK_DIPR3 0x0f		/**< Socket Dest IP Octet 3 (LSB) */
#define SOCK_DPORT0 0x10	/**< Socket Dest Port (MSB) */
#define SOCK_DPORT1 0x11	/**< Socket Dest Port (LSB) */
#define SOCK_MSSR0 0x12		/**< Socket MSS (MSB) */
#define SOCK_MSSR1 0x13		/**< Socket MSS (LSB) */
/* 0x14 reserved */
#define SOCK_TOS 0x15		/**< Socket IP TOS */
#define SOCK_TTL 0x16		/**< Socket IP TTL */
/* 0x17 - 0x1d reserved */
#define SOCK_RXBUF_SIZE 0x1e /**< Socket RX Buffer Size */
#define SOCK_TXBUF_SIZE 0x1f /**< Socket TX Buffer Size */
#define SOCK_TX_FSR0 0x20	/**< Socket TX Free Size (MSB) */
#define SOCK_TX_FSR1 0x21	/**< Socket TX Free Size (LSB) */
#define SOCK_TX_RD0 0x22	/**< Socket TX Read Pointer (MSB) */
#define SOCK_TX_RD1 0x23	/**< Socket TX Read Pointer (LSB) */
#define SOCK_TX_WR0 0x24	/**< Socket TX Write Pointer (MSB) */
#define SOCK_TX_WR1 0x25	/**< Socket TX Write Pointer (LSB) */
#define SOCK_RX_RSR0 0x26	/**< Socket RX Received Size (MSB) */
#define SOCK_RX_RSR1 0x27	/**< Socket RX Received Size (LSB) */
#define SOCK_RX_RD0 0x28	/**< Socket RX Read Pointer (MSB) */
#define SOCK_RX_RD1 0x29	/**< Socket RX Read Pointer (LSB) */
#define SOCK_RX_WR0 0x2a	/**< Socket RX Write Pointer (MSB) */
#define SOCK_RX_WR1 0x2b	/**< Socket RX Write Pointer (LSB) */
#define SOCK_IMR 0x2c		/**< Socket Interrupt Mask */
#define SOCK_FRAG0 0x2d		/**< Socket Fragment Offset IP Hdr (MSB) */
#define SOCK_FRAG1 0x2e		/**< Socket Fragment Offset IP Hdr (LSB) */
#define SOCK_KPALVTR 0x2f	/**< Socket Keepalive Timer */
/* 0x30 - 0xffff reserved */

/* W5500 Common Register Bits */
#define COM_MR_RST 0x80		/**< MR: Reset */
#define COM_MR_WOL 0x20 	/**< MR: WOL Enable */
#define COM_MR_PB 0x10		/**< MR: Ping Block Mode */
#define COM_MR_PPPOE 0x08	/**< MR: PPPoE Enable */
#define COM_MR_FARP 0x01	/**< MR: Force ARP */
#define COM_IR_CONFLICT 0x80 /**< IR: IP Conflict */
#define COM_IR_UNREACH 0x40	/**< IR: Destination Unreachable */
#define COM_IR_PPPOE 0x20	/**< IR: PPPoE Connection Close */
#define COM_IR_MP 0x10		/**< IR: WOL Magic Packet */
#define COM_IM_IR7 0x80		/**< IMR: IP Conflict Mask */
#define COM_IM_IR6 0x40		/**< IMR: Destination Unreachable Mask */
#define COM_IM_IR5 0x20		/**< IMR: PPPoE Close Mask */
#define COM_IM_IR4 0x10		/**< IMR: Magic Packet Mask */
#define COM_PHYCFGR_RST 0x80 /**< PHYCFGR: Reset PHY */
#define COM_PHYCFGR_OPMD 0x40 /**< PHYCFGR: Select PHY Mode Source */
#define COM_PHYCFGR_OPMDC2 0x20 /**< PHYCFGR: PHY Op Mode Bit 2 */
#define COM_PHYCFGR_OPMDC1 0x10 /**< PHYCFGR: PHY Op Mode Bit 1 */
#define COM_PHYCFGR_OPMDC0 0x08 /**< PHYCFGR: PHY Op Mode Bit 0 */
#define COM_PHYCFGR_DPX 0x04 /**< PHYCFGR: Duplex (RO) */
#define COM_PHYCFGR_SPD 0x02 /**< PHYCFGR: Speed (RO) */
#define COM_PHYCFGR_LNK 0x01 /**< PHYCFGR: Link (RO) */

/* PHY Config values */
#define COM_PHYCFG_OPMDC_bp 3	/**< Bit offset to PHY mode setting */
#define COM_PHYCFG_OPMDC_gm 0x38 /**< Mask for PHY mode setting */
#define PHY_AUTO 0x7		/**< PHY Mode: Autonegotiate */
#define PHY_OFF 0x6			/**< PHY Mode: Power Down */
#define PHY_100HALF 0x4		/**< PHY Mode: 100M Half, No Auto */
#define PHY_100FULL 0x3		/**< PHY Mode; 100M Full, No Auto */
#define PHY_100HALFB 0x2	/**< PHY Mode: 100M Half, No Auto (dupe?) */
#define PHY_10FULL 0x1		/**< PHY Mode: 10M Full, No Auto */
#define PHY_10HALF 0x0		/**< PHY Mode: 10M Half, No Auto */

/* Socket Register Bits */
#define SOCK_MR_MULTI 0x80	/**< Sock MR: Multicast in UDP mode */
#define SOCK_MR_MFEN 0x80 	/**< Sock MR: MAC Filter in RAW mode */
#define SOCK_MR_BCASTB 0x40	/**< Sock MR: Broadcast Block in UDP/RAW */
#define SOCK_MR_ND 0x20		/**< Sock MR: No Delayed ACK in TCP mode */
#define SOCK_MR_MC 0x20 	/**< Sock MR: IGMP v1 in UDP Multicast (0=v2) */
#define SOCK_MR_MMB 0x20	/**< Sock MR: Multicast Block in RAW */
#define SOCK_MR_UCASTB 0x10 /**< Sock MR: Unicast Block in UDP Multicast */
#define SOCK_MR_MIP6B 0x10	/**< Sock MR: IPv6 Block in RAW mode */
#define SOCK_MR_MACRAW 0x04	/**< Sock MR: Mode = MACRAW */
#define SOCK_MR_UDP 0x02    /**< Sock MR: Mode = UDP */
#define SOCK_MR_TCP 0x01    /**< Sock MR: Mode = TCP */
#define SOCK_MR_PROTO_gm 0xf /**< Mask for Socket Mode */
#define SOCK_CR_OPEN 0x01	/**< Sock CR: Command Open */
#define SOCK_CR_LISTEN 0x02	/**< Sock CR: Command Listen (TCP) */
#define SOCK_CR_CONNECT 0x04 /**< Sock CR: Command Connect (TCP) */
#define SOCK_CR_DISCON 0x08 /**< Sock CR: Command Disconnect (TCP) */
#define SOCK_CR_CLOSE 0x10  /**< Sock CR: Command Close */
#define SOCK_CR_SEND 0x20   /**< Sock CR: Send */
#define SOCK_CR_SENDMAC 0x21 /**< Sock CR: Send (pre-defined MAC) (UDP) */
#define SOCK_CR_SENDKEEP 0x22 /**< Sock CR: Send Keepalive (TCP) */
#define SOCK_CR_RECV 0x40	/**< Sock CR: Complete Recieve */
#define SOCK_IR_SENDOK 0x10 /**< Sock IR: Send Okay */
#define SOCK_IR_TIMEOUT 0x08 /**< Sock IR: ARP/TCP timeout */
#define SOCK_IR_RECV 0x04 	/**< Sock IR: Data RX */
#define SOCK_IR_DISCON 0x02 /**< Sock IR: Far End Disconnect */
#define SOCK_IR_CON 0x01	/**< Sock IR: Connection Established */
#define SOCK_SR_CLOSED 0x00	/**< Sock SR: Closed */
#define SOCK_SR_INIT 0x13	/**< Sock SR: Init (TCP) */
#define SOCK_SR_LISTEN 0x14	/**< Sock SR: Listen (TCP) */
#define SOCK_SR_ESTABLISHED 0x17 /**< Sock SR: Established (TCP) */
#define SOCK_SR_CLOSEWAIT 0x1c /**< Sock SR: Close Wait (TCP) */
#define SOCK_SR_UDP 0x22	/**< Sock SR: UDP Mode */
#define SOCK_SR_MACRAW 0x42	/**< Sock SR: MACRAW Mode */
#define SOCK_SR_SYNSENT 0x15 /**< Sock SR: SYN sent (TCP) */
#define SOCK_SR_SYNRECV 0x16 /**< Sock SR: SYN received (TCP) */
#define SOCK_SR_FINWAIT 0x18 /**< Sock SR: FIN wait 1 (TCP) */
#define SOCK_SR_CLOSING 0x1a /**< Sock SR: FIN wait 2 (TCP) */
#define SOCK_SR_TIMEWAIT 0x1b /**< Sock SR: Time wait (TCP) */
#define SOCK_SR_LASTACK 0x1d /**< Sock SR: Last ACK (TCP) */
#define SOCK_IMR_SENDOK 0x10 /**< Sock IMR: Send Okay Mask */
#define SOCK_IMR_TIMEOUT 0x08 /**< Sock IMR: Timeout Mask */
#define SOCK_IMR_RECV 0x04	/**< Sock IMR: Recieve Mask */
#define SOCK_IMR_DISCON 0x02 /**< Sock IMR: Disconnect Mask */
#define SOCK_IMR_CON 0x01 /**< Sock IMR: Connect Mask */

uint8_t w5500_pin; /**< CS Pin internal setting */
PORT_t *w5500_port; /**< CS Pin Port internal setting */
uint8_t w5500_spi;

#define cs_select w5500_port->OUTCLR = w5500_pin;
#define cs_end w5500_port->OUTSET = w5500_pin;

/* private prototypes */
uint8_t _read_reg(uint8_t block, uint16_t address);
void _write_reg(uint8_t block, uint16_t address, uint8_t value);
uint16_t _read_reg16(uint8_t block, uint16_t address);
void _write_reg16(uint8_t block, uint16_t address, uint16_t value);
void _write_block(uint8_t block, uint16_t address, uint8_t len,
		uint8_t *values);
void _read_block(uint8_t block, uint16_t address, uint8_t len,
		uint8_t *values);
uint16_t _find_free_port(void);
int _sock_tcp_put(char s, FILE *handle);
int _sock_tcp_get(FILE *handle);

/* maximum number of sockets the chip supports */
#define W5500_MAX_SOCKETS 8

typedef enum {
    S_UNPREP = 0, /**< socket is unprepared */
    S_CLOSED, /**< socket is closed */
    S_LISTEN, /**< socket has been opened for listening */
    S_ESTAB, /**< socket has established connection */
} w5500_sockstate_t;

/* internal structures */
typedef struct {
    w5500_sockstate_t state; /* is this socket currently in use */
    uint8_t socknum; /* socket number, used mostly in stdio code */
    uint16_t cwp; /* cached current write address, relative to socket */
    uint16_t crp; /* cached current read address, relative to socket */
    uint16_t port; /* the port on our end used by this, so we don't conflict */
    uint8_t *txbuf; /* pointer to the tx buffer used by stdio mapping */
    uint8_t *rxbuf; /* pointer to the rx buffer used by stdio mapping */
    uint16_t btxlen; /* how many characters we are currently buffering for TX */
    uint16_t brxlen; /* how many characters we are currently buffering for RX */
    uint16_t brp; /* rx pointer into buf */
    uint16_t btp; /* tx pointer into buf */
    uint16_t buflen; /* the size of the buffers above */
    uint16_t read; /* bytes read from the chip buffer */
    /* probably something else */
} w5500_socket_t;

w5500_socket_t _socktable[W5500_MAX_SOCKETS]; /* this is just a fixed constant */

uint8_t _read_reg(uint8_t block, uint16_t address) {
	uint8_t buf[4], rxbuf[4];

	if (!w5500_port) {
		/* nothing much we can do */
		return 0;
	}
	buf[0] = address >> 8;
    buf[1] = address & 0xff;
    buf[2] = block;
    buf[3] = 0;

	cs_select;
	spi_txrx(w5500_spi,buf,rxbuf,4);
	cs_end;
#ifdef DEBUG_W5500
	printf_P(PSTR("w5500: [%02x]%04x->%02x\r\n"),block,address,buf[3]);
#endif // DEBUG_W5500
	return rxbuf[3];
}

uint16_t _read_reg16(uint8_t block, uint16_t address) {
	uint8_t buf[5], rxbuf[5];
	uint16_t value;

	if (!w5500_port) {
		/* nothing much we can do */
		return 0;
	}
	buf[0] = address >> 8;
    buf[1] = address & 0xff;
    buf[2] = block;
    buf[3] = 0;
    buf[4] = 0;

    /* apparently, as we are reading a 16-bit register, we have to multiread it
        until we get the same value twice */

    /* read first instance */
	cs_select;
	spi_txrx(w5500_spi,buf,rxbuf,5);
	cs_end;

	/* initial value we got from far end */
	value = (rxbuf[3] << 8) | rxbuf[4];

	/* do successive reads until the value is stable */
	while (1) {
        cs_select;
        spi_txrx(w5500_spi,buf,rxbuf,5);
        cs_end;
        if (((rxbuf[3] << 8) | rxbuf[4]) == value) {
            break;
        }
        value = ((rxbuf[3] << 8) | rxbuf[4]);
	}

#ifdef DEBUG_W5500
	printf_P(PSTR("w5500: [%02x]%04x->%04x\r\n"),block,address,value);
#endif // DEBUG_W5500
	return value;
}

void _write_reg(uint8_t block, uint16_t address, uint8_t value) {
    uint8_t buf[4];

	if (!w5500_port) {
		return;
	}
    buf[0] = address >> 8;
    buf[1] = address & 0xff;
    buf[2] = block | RWB;
    buf[3] = value;

	cs_select;
	spi_txrx(w5500_spi,buf,NULL,4); /* discard what comes back */
	cs_end;

#ifdef DEBUG_W5500
	printf_P(PSTR("w5500: %02x->[%02x]%04x\r\n"),value,block,address);
#endif // DEBUG_W5500
	return;
}

void _write_reg16(uint8_t block, uint16_t address, uint16_t value) {
    uint8_t buf[5];

	if (!w5500_port) {
		return;
	}
    buf[0] = address >> 8;
    buf[1] = address & 0xff;
    buf[2] = block | RWB;
    buf[3] = value >> 8;
    buf[4] = value & 0xff;

	cs_select;
	spi_txrx(w5500_spi,buf,NULL,5); /* discard what comes back */
	cs_end;

#ifdef DEBUG_W5500
	printf_P(PSTR("w5500: %02x%02x->[%02x]%04x\r\n"),value >> 8, value &0xff,block,address);
#endif // DEBUG_W5500
	return;
}

void _write_block(uint8_t block, uint16_t address, uint8_t len,
		uint8_t *values) {
    uint8_t buf[3];

	if (!w5500_port) {
		return;
	}
    buf[0] = address >> 8;
    buf[1] = address & 0xff;
    buf[2] = block | RWB;
	cs_select;
	spi_txrx(w5500_spi,buf,NULL,3); /* issue write command */
	spi_txrx(w5500_spi,values,NULL,len); /* and now the whole buffer */
	cs_end;
	return;
}

void _read_block(uint8_t block, uint16_t address, uint8_t len,
		uint8_t *values) {
    uint8_t buf[3];

	if (!w5500_port) {
		return;
	}
    buf[0] = address >> 8;
    buf[1] = address & 0xff;
    buf[2] = block;
	cs_select;
	spi_txrx(w5500_spi,buf,NULL,3); /* issue read command */
	spi_txrx(w5500_spi,NULL,values,len); /* and now fill inbound buffer */
	cs_end;
	return;
}

/* proper implemetation begin */
int w5500_init(spi_portname_t spi_port, PORT_t *cs_port, uint8_t cs_pin,
		uint8_t *mac) {
	uint8_t ver, i;
	uint8_t ip[] = {192,168,1,1};
	uint8_t regtest[] = {192,168,1,1};

    /* nuke the socket status area */
    memset(&_socktable,0,sizeof(w5500_socket_t)*W5500_MAX_SOCKETS);

    /* set up the SPI details */
	w5500_port = cs_port;
	w5500_pin = cs_pin;
	w5500_spi = spi_port;

	/* ensure CS is deselected before playing with the port */
	w5500_port->DIRSET = w5500_pin;
	cs_end;

	/* init the SPI port */
	spi_init(spi_port);
	spi_conf(spi_port,spi_perdiv2,spi_mode0);

	/* validate that we're talking to the chip we expect */
	ver = _read_reg(BLK_COMMON,COM_VERSIONR);

	if (ver != 4) {
		printf_P(PSTR("w5500: failed init\r\n"));
		w5500_port = NULL;
		return -ENODEV;
	}

	/* write some junk to it, and then check to see it disappears on reset */
	_write_block(BLK_COMMON,COM_GAR0,4,ip);
	_read_block(BLK_COMMON,COM_GAR0,4,ip);
#ifdef DEBUG_W5500
	printf_P(PSTR("reg: %d.%d.%d.%d\r\n"),ip[0],ip[1],ip[2],ip[3]);
#endif // DEBUG_W5500
	/* compare it to make sure it's been set properly */
	if (!memcmp(ip,regtest,6)) {
		/* we failed a clean readback for the chip */
		printf_P(PSTR("w5500: readback failed, aborting\r\n"));
		w5500_port = NULL;
		return -ENODEV;
	}
	/* now ensure it goes away on reset */
	memset(regtest,0,6);
	/* write a reset to it */
	_write_reg(BLK_COMMON,COM_MR,COM_MR_RST);
	while (_read_reg(BLK_COMMON,COM_MR) & COM_MR_RST);
	/* read back the GAR0 register, should be all zeros */
	_read_block(BLK_COMMON,COM_GAR0,4,ip);
#ifdef DEBUG_W5500
	printf_P(PSTR("reg: %d.%d.%d.%d\r\n"),ip[0],ip[1],ip[2],ip[3]);
#endif // DEBUG_W5500
	if (!memcmp(ip,regtest,6)) {
		/* failed to reset the chip */
		printf_P(PSTR("w5500: reset failed, aborting\r\n"));
		w5500_port = NULL;
		return -ENODEV;
	}

    /* set the buffer size on every socket to 2kB */
    for (i = 0; i < W5500_MAX_SOCKETS; i++) {
        _write_reg(BLK_SOCKET_REG(i),SOCK_RXBUF_SIZE,2);
        _write_reg(BLK_SOCKET_REG(i),SOCK_TXBUF_SIZE,2);
//        printf_P(PSTR("%d:rx=%d;tx=%d\r\n"),i,
//            _read_reg(BLK_SOCKET_REG(i),SOCK_RXBUF_SIZE),
//            _read_reg(BLK_SOCKET_REG(i),SOCK_TXBUF_SIZE));
    }

	/* write the MAC to the approprate registers */
	_write_block(BLK_COMMON,COM_SHAR0,6,mac);

	/* we're done */
	return 0;
}

/* configure IP address settings on the chip */
int w5500_ip_conf(uint8_t *ip, uint8_t cidr, uint8_t *gw) {
	uint32_t mask;

	/* check we have the port first */
	if (!w5500_port) {
		return -ENODEV;
	}
	/* compute mask from cidr length */
	mask = (0xffff << (32-cidr));

	/* write the details to the common register */
	_write_block(BLK_COMMON,COM_SIPR0,4,ip);
	_write_block(BLK_COMMON,COM_SUBR0,4,(uint8_t *)&mask);
	_write_block(BLK_COMMON,COM_GAR0,4,gw);

	return 0;
}

int w5500_socket_init(uint8_t socknum, uint16_t rxsize, uint16_t txsize) {
    /* toss invalid socket numbers */
    if (socknum > W5500_MAX_SOCKETS) {
        return -EINVAL;
    }

    /* now check to see the socket is not already in in use */
    if (_socktable[socknum].state != S_CLOSED &&
        _socktable[socknum].state != S_UNPREP) {
        printf_P(PSTR("w5500: socket in use, cannot (re-)init\r\n"));
        return -EBUSY;
    }

    /* check to see the underlying hardware is actually closed */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_CLOSED) {
        printf_P(PSTR("w5500: hw socket not closed, no init\r\n"));
        return -EBUSY;
    }

    /* mark the socket as closed, since that allows it to be used */
    _socktable[socknum].state = S_CLOSED;

    /* store the block used for the socket, this is primarily in stdio land */
    _socktable[socknum].socknum = socknum;

    /* configure the socket on the chip as per definitions above */
    //_write_reg(BLK_SOCKET_REG(socknum),SOCK_RXBUF_SIZE,rxsize);
    //_write_reg(BLK_SOCKET_REG(socknum),SOCK_TXBUF_SIZE,txsize);

    /* we're all done, get out of there */
    return 0;
}

int w5500_socket_listen(uint8_t socknum, uint16_t port) {

}

int w5500_tcp_connect(uint8_t socknum, uint8_t *addr, uint16_t port) {
    uint16_t sport;
    uint16_t x;

    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS) {
        return -EINVAL;
    }
    /* make sure we're in the right state */
    if (_socktable[socknum].state != S_CLOSED) {
        printf_P(PSTR("w5500: socket not ready for connect\r\n"));
        return -ENOTREADY;
    }

    /* check to see the socket is actually closed at this point */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_CLOSED) {
        printf_P(PSTR("w5500: hw socket busy\r\n"));
        return -EBUSY;
    }

    /* set the mode to TCP */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_MR,SOCK_MR_TCP);

    /* first set up the source end to go to open */
    sport = _find_free_port();
    _socktable[socknum].port = sport;
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_PORT0,sport);

    /* open the socket and wait for that to be complete */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_OPEN);
    while (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_INIT);
    /* null body */

    /* set the target addresses and port */
    _write_block(BLK_SOCKET_REG(socknum),SOCK_DIPR0,4,addr);
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_DPORT0,port);

    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,0xff); /* clear all interrupts */

    /* now issue the connect */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_CONNECT);

    /* watch the interrupt reg for change of state */
    while (!((x = _read_reg(BLK_SOCKET_REG(socknum),SOCK_IR)) & (SOCK_IR_CON | SOCK_IR_TIMEOUT)));
    /* null body */
    /* clear the flags since we will just check it via the SOCK_SR register */

    //printf_P(PSTR("s=%x,now=%x\r\n"),x,_read_reg(BLK_SOCKET_REG(socknum),SOCK_IR));

    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,(SOCK_IR_CON | SOCK_IR_TIMEOUT));

    //x = _read_reg(BLK_SOCKET_REG(socknum),SOCK_SR);
    //printf_P(PSTR("sr=%x"),x);

    /* either we got a timeout or we got an established connection */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) == SOCK_SR_ESTABLISHED) {
        /* all is good! return it's fine */
        _socktable[socknum].read = 0; /* no bytes read yet */
        _socktable[socknum].state = S_ESTAB;
        return 0;
    }

    /* restore state to closed */

    /* figure out if we got a destination unreachable or not */
    if (_read_reg(BLK_COMMON,COM_IR) && COM_IR_UNREACH) {
        printf_P(PSTR("w5500: icmp unreachable\r\n"));
        _write_reg(BLK_COMMON,COM_IR,COM_IR_UNREACH);
        return -EHOSTUNREACH;
    }

    /* we don't know, it just didn't work */
    printf_P(PSTR("w5500: connect timeout\r\n"));
    return -ETIME;

}

/* close a socket */
int w5500_tcp_close(uint8_t socknum) {

    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS) {
        return -EINVAL;
    }

    /* make sure we're in the right state */
    /* we should be in listen or estab */
    /* allow us to close sockets in any internal state */
    //if (_socktable[socknum].state == S_CLOSED) {
    //    printf_P(PSTR("w5500: can't close socket not in use\r\n"));
    //    return -EINVAL;
    //}

    /* check to see we're not actually working on a closed socket already */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) == SOCK_SR_CLOSED) {
        printf_P(PSTR("w5500: hw socket already closed\r\n"));
        _socktable[socknum].state = S_CLOSED;
        return -EINVAL;
    }

    /* clear interrupts */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,0xff);
    /* command close */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_DISCON);

    /* wait for socket to complete closing */
    while (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_CLOSED);
    /* null body */

    /* update our internal status */
    _socktable[socknum].state = S_CLOSED;

    return 0;
}

int _sock_tcp_put(char s, FILE *handle) {
    w5500_socket_t *sock;
    uint16_t chip_txfree; /* how much space is in the chip tx buf */
    uint16_t ctxwp; /* chip's tx write pointer */
    uint16_t mss; /* MSS as reported by chip */
    uint16_t ctxrp; /* chip's tx read pointer */

    sock = (w5500_socket_t *)fdev_get_udata(handle);
    if (!sock) {
        return _FDEV_ERR;
    }

    /* socket is still okay, right? */
    if (sock->state != S_ESTAB) {
        return _FDEV_ERR;
	}

    /* sanity, someone diff malloc us some buffers right? */
    if (!sock->txbuf) {
        return _FDEV_ERR;
    }

    /* check to see if the socket has been closed on us */
    if (_read_reg(BLK_SOCKET_REG(sock->socknum),SOCK_SR) != SOCK_SR_ESTABLISHED) {
        /* close it */
        printf_P(PSTR("w5500: lost connection\r\n"));
        w5500_tcp_close(sock->socknum);
        return _FDEV_ERR;
    }

    /* so, see if we need to flush the buffer yet */
    if (sock->btxlen == sock->buflen) {
        /* check to see the socket can accept another buflen bytes */
        chip_txfree = _read_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_TX_FSR0);

        /* work out if we can clear some space */
        if (chip_txfree < sock->buflen) {
            /* nope, so oh well, you can't write at the moment */
            printf_P(PSTR("w5500: tx buffer overrun\r\n"));
            return _FDEV_ERR;
        }
        /* send the bulk data, and increment the write pointer to match */
        ctxwp = _read_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_TX_WR0);
        _write_block(BLK_SOCKET_TX(sock->socknum),ctxwp,sock->buflen,sock->txbuf);
        ctxwp += sock->buflen;
        _write_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_TX_WR0,ctxwp);
        /* remove our buffered items */
        sock->btp = 0;
        sock->btxlen = 0;
        /* work out if the waiting data is larger than the socket MSS */
        mss = _read_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_MSSR0);
        ctxrp = _read_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_TX_RD0);
        if (ctxrp > sock->cwp) {
            if (mss < ((0xffff - ctxrp) + sock->cwp)) {
                /* we should send, do so */
                /* clear interrupt before doing this */
                _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_IR,SOCK_IR_SENDOK);
                /* do the send */
                _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_CR,SOCK_CR_SEND);
                /* wait for send complete, before moving on */
                while (!(_read_reg(BLK_SOCKET_REG(sock->socknum),SOCK_IR) & SOCK_IR_SENDOK));
                /* clear interupt when done */
                _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_IR,SOCK_IR_SENDOK);
            }
            /* not enough to send, just ignore it */
        } else {
            if (mss < (sock->cwp - ctxrp)) {
                /* we should send, do so */
                /* clear interrupt before doing this */
                _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_IR,SOCK_IR_SENDOK);
                /* do the send */
                _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_CR,SOCK_CR_SEND);
                /* wait for send complete, before moving on */
                while (!(_read_reg(BLK_SOCKET_REG(sock->socknum),SOCK_IR) & SOCK_IR_SENDOK));
                /* clear interupt when done */
                _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_IR,SOCK_IR_SENDOK);
            }
        }
    }

    /* put the character into the tx buffer, next call will really send it if this is
       a full buffer */
    sock->txbuf[sock->btp] = s;
    sock->btp++;
    sock->btxlen++;

    return 1;
}

/* force a TCP connection to push data */
int w5500_tcp_push(uint8_t socknum) {
    uint16_t chip_txfree, ctxwp;
    uint8_t stat;

    chip_txfree = _read_reg16(BLK_SOCKET_REG(socknum),SOCK_TX_FSR0);

    /* work out if we can clear some space */
    if (chip_txfree < _socktable[socknum].btxlen) {
        /* nope, so oh well, you can't write at the moment */
        return -EIO;
    }

    /* check to see we're not trying to push a closed socket */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_ESTABLISHED) {
        printf_P(PSTR("w5500: lost connection\r\n"));
        w5500_tcp_close(0);
        return -EIO;
    }

    /* send the bulk data, and increment the write pointer to match */
    ctxwp = _read_reg16(BLK_SOCKET_REG(socknum),SOCK_TX_WR0);
    _write_block(BLK_SOCKET_TX(socknum),ctxwp,_socktable[socknum].btxlen,
                _socktable[socknum].txbuf);
    ctxwp += _socktable[socknum].btxlen;
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_TX_WR0,ctxwp);

    /* remove our buffered items */
    _socktable[socknum].btp = 0;
    _socktable[socknum].btxlen = 0;

    /* now force the send to take place */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,SOCK_IR_SENDOK);
    /* do the send */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_SEND);
    /* wait for send complete, before moving on */
    while (!((stat = _read_reg(BLK_SOCKET_REG(socknum),SOCK_IR)) & (SOCK_IR_SENDOK | SOCK_IR_DISCON | SOCK_IR_TIMEOUT)));
    /* clear interupt when done */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,SOCK_IR_SENDOK | SOCK_IR_DISCON | SOCK_IR_TIMEOUT);
    /* work out why we exited */
    if (stat & SOCK_IR_DISCON || stat & SOCK_IR_TIMEOUT) {
        printf_P(PSTR("w5500: lost connection\r\n"));
        w5500_tcp_close(0);
        return -EIO;
    }

    /* all good, return */
    return 0;
}

int _sock_tcp_get(FILE *handle) {
	w5500_socket_t *sock;
	uint8_t c; /* the char */
	uint16_t chip_rxlen; /* how much data the chip thinks it has left to read */
	uint16_t crxrp; /* chip's read pointer */

	/* reteieve the pointer to the struct for our hardware */
	sock = (w5500_socket_t *)fdev_get_udata(handle);
	if (!sock) {
		return _FDEV_ERR; /* avr-libc doesn't describe this, but never mind */
	}

	if (sock->state != S_ESTAB) {
        return _FDEV_ERR;
	}

	/* sanity, someone did malloc us some buffers, right? */
	if (!sock->rxbuf) {
        return _FDEV_ERR;
	}

    c = _read_reg(BLK_SOCKET_REG(sock->socknum),SOCK_SR);
    switch (c) {
        /* only these states are useful for a TCP read */
        case SOCK_SR_CLOSEWAIT: /* half-closed from far end, but buffer to read */
        case SOCK_SR_ESTABLISHED: /* normal open connection */
        case SOCK_SR_FINWAIT: /* we've half-closed, may still be readable */
            break;
        default:
            /* close it */
            printf_P(PSTR("w5500: read from closed socket\r\n"));
            w5500_tcp_close(sock->socknum);
            return _FDEV_ERR;
            break;
    }

	/* now work out what we can read from the buffers, or retrieve from the chip */
	if (!(sock->brxlen)) {
        /* we have nothing buffered, attempt to read up to buflen chars from the chip */

        /* FIXME: the RSR0 register is only updated on RECV, so we have to work out
         * what is left directly from our own pointer copies. this would avoid
         * many window updates on the wire */
        chip_rxlen = _read_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_RX_RSR0);

        //printf_P(PSTR("%04x\r\n"),chip_rxlen);

        if (chip_rxlen == 0) {
            /* force an ACK, just in case */
            _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_CR,SOCK_CR_RECV);
            /* return end of stream */
            //printf_P(PSTR("w5500: end of stream"));
            return _FDEV_EOF;
        }

        /* we now know how many bytes are waiting in the socket, read up to buflen of them */
        if (chip_rxlen > sock->buflen) {
            chip_rxlen = sock->buflen;
        } else {
            /* running out of data to read, get some more */
           // _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_CR,SOCK_CR_RECV);
        }
        /* read into the buffer unread data */
        /* we always get the current pointer, just in case */
        crxrp = _read_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_RX_RD0);
        _read_block(BLK_SOCKET_RX(sock->socknum),crxrp,chip_rxlen,sock->rxbuf);
        /* update various things */
        crxrp += chip_rxlen;
        /* update the read pointer on-chip and accept the data */
        _write_reg16(BLK_SOCKET_REG(sock->socknum),SOCK_RX_RD0,crxrp);
        /* apparently, we have to ack every update to the buffer, or else it goes
           nuts *shrug* */
        _write_reg(BLK_SOCKET_REG(sock->socknum),SOCK_CR,SOCK_CR_RECV);

        /* and keep track of what we're just read */
        sock->brxlen = chip_rxlen;
        sock->brp = 0;
	}
	/* we have some characters buffered, return the next one */
	c = sock->rxbuf[sock->brp];
	sock->brp++;
	sock->brxlen--;

	/* and we're done! */
	return c;
}

uint16_t w5500_tcp_unread(uint8_t socknum) {
    uint16_t chip_rxlen;

    /* fixme: this relies on RSR0 being updated, which only happens on RECV command */
    chip_rxlen = _read_reg16(BLK_SOCKET_REG(socknum),SOCK_RX_RSR0);
    return (_socktable[socknum].brxlen + chip_rxlen);
}

FILE *w5500_tcp_map_stdio(uint8_t socknum, uint16_t bufsize) {
    FILE *handle = NULL;
    uint8_t *buf; /* used to initalise the buffers */

    handle = fdevopen(&_sock_tcp_put,&_sock_tcp_get);
    if (!handle) {
        return NULL;
    }

    /* allocate space for the buffers and initalise the pointers */
    buf = malloc(bufsize);
    if (!buf) {
        return NULL;
    }
    _socktable[socknum].txbuf = buf;
    buf = malloc(bufsize);
    if (!buf) {
        free(_socktable[socknum].txbuf);
        return NULL;
    }
    _socktable[socknum].rxbuf = buf;
    _socktable[socknum].brp = 0;
    _socktable[socknum].btp = 0;
    _socktable[socknum].buflen = bufsize;
    _socktable[socknum].brxlen = 0;
    _socktable[socknum].btxlen = 0;

    fdev_set_udata(handle,(void *)&_socktable[socknum]);
	return handle;
}

int w5500_udp_listen(uint8_t socknum, uint16_t port) {
    uint8_t n;

    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS) {
        return -EINVAL;
    }
    /* make sure we're in the right state */
    if (_socktable[socknum].state != S_CLOSED) {
        printf_P(PSTR("w5500: socket not ready for connect\r\n"));
        return -ENOTREADY;
    }

    /* scan the list of sports we have, check it doesn't collide */
    for (n = 0; n < W5500_MAX_SOCKETS; n++) {
        if (_socktable[n].port == port) {
            /* fail! */
            printf_P(PSTR("w5500: port in use\r\n"));
            return -EBUSY;
        }
    }

    /* check to see the socket is actually closed at this point */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_CLOSED) {
        printf_P(PSTR("w5500: hw socket busy\r\n"));
        return -EBUSY;
    }

    /* set the mode to UDP */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_MR,SOCK_MR_UDP);

    /* write the source port */
    _socktable[socknum].port = port;
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_PORT0,port);

    /* open the socket and wait for that to be complete */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_OPEN);
    while (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_UDP);

    _socktable[socknum].state = S_ESTAB;
    _socktable[socknum].brxlen = 0; /* these are re-used by UDP for packet len */
    _socktable[socknum].btxlen = 0;
    /* we're now good to exchange packets over UDP */

    return 0;
}

int w5500_udp_close(uint8_t socknum) {
    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS) {
        return -EINVAL;
    }

    /* check to see we're not actually working on a closed socket already */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_UDP) {
        printf_P(PSTR("w5500: hw socket not udp\r\n"));
        /* don't touch the socket */
        return -EINVAL;
    }

    /* command close, we don't need to issue DISCON since it's stateless */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_CLOSE);

    /* wait for socket to complete closing */
    while (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_CLOSED);
    /* null body */

    /* update our internal status */
    _socktable[socknum].state = S_CLOSED;

    return 0;
}

int w5500_udp_rxmeta(uint8_t socknum, uint8_t *ip, uint16_t *port, uint16_t *len) {
    uint8_t buf[8];
    uint16_t crxrp;

    if (!ip || !port || !len) {
        return -EINVAL;
    }

    /* check to see if there's anything to read */
    if (_read_reg16(BLK_SOCKET_REG(socknum),SOCK_RX_RSR0) == 0) {
        return -ENOTREADY;
    }

    /* read 8 bytes */
    crxrp = _read_reg16(BLK_SOCKET_REG(socknum),SOCK_RX_RD0);
    _read_block(BLK_SOCKET_RX(socknum),crxrp,8,buf);
    /* update the pointer, and ack it even tho we don't send back anything */
    crxrp += 8;
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_RX_RD0,crxrp);
    /* release space for further packets */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_RECV);

    memcpy(ip,buf,4);
    *port = (buf[4] << 8 | buf[5]);
    *len = (buf[6] << 8 | buf[7]);
    /* copy len into the socktable */
    /* we re-use brxlen for this */
    _socktable[socknum].brxlen = *len;

    return 0;
}

int w5500_udp_read(uint8_t socknum, uint16_t len, uint8_t *buf) {
    uint16_t crxrp;
    /* the UDP code doesn't use the socktable buffers because
     * the vast majority of occasions we'll be reading whole
     * packets into structures. tcp also uses the single-char
     * avr-libc stream interface, so the buffers are needed to
     * avoid entirely single-char read/write */

    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS || !buf) {
        return -EINVAL;
    }

    /* make sure we're in the right state */
    if (_socktable[socknum].state != S_ESTAB) {
        printf_P(PSTR("w5500: read from closed socket\r\n"));
        return -EIO;
    }

    /* check to see if the socket has been closed on us */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_UDP) {
        /* close it */
        printf_P(PSTR("w5500: read from closed socket\r\n"));
        return -EIO;
    }

    /* we need to use the cached copy of the PACKET length, since RSR0
     * is not accurate for UDP packets */

    if (_socktable[socknum].brxlen < len) {
        if (_socktable[socknum].brxlen == 0) {
            return 0; /* nothing to read */
        }
        len = _socktable[socknum].brxlen;
    }

    crxrp = _read_reg16(BLK_SOCKET_REG(socknum),SOCK_RX_RD0);
    if (buf) {
        _read_block(BLK_SOCKET_RX(socknum),crxrp,len,buf);
    }
    /* update the pointer, and ack it even tho we don't send back anything */
    crxrp += len;
    _socktable[socknum].brxlen -= len;
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_RX_RD0,crxrp);
    /* release space for further packets */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_RECV);

    return len; /* tell the caller how much they got */
}

int w5500_udp_write(uint8_t socknum, uint16_t len, uint8_t *buf) {
    uint16_t ctxwp;

    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS || !buf) {
        return -EINVAL;
    }

    /* make sure we're in the right state */
    if (_socktable[socknum].state != S_ESTAB) {
        printf_P(PSTR("w5500: read from closed socket\r\n"));
        return -EIO;
    }

    /* check to see if the socket has been closed on us */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_UDP) {
        /* close it */
        printf_P(PSTR("w5500: read from closed socket\r\n"));
        return -EIO;
    }

    /* check to see if we have space to write this anyway */
    if (_read_reg16(BLK_SOCKET_REG(socknum),SOCK_TX_FSR0)-_socktable[socknum].btxlen < len) {
        /* no space for you! */
        return -EIO;
    }

    /* write the block out */
    /* send the bulk data, and increment the write pointer to match */
    ctxwp = _read_reg16(BLK_SOCKET_REG(socknum),SOCK_TX_WR0);
    _write_block(BLK_SOCKET_TX(socknum),ctxwp,len,buf);
    ctxwp += len;
    _socktable[socknum].btxlen += len;
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_TX_WR0,ctxwp);

    /* actual send happens later */
    return 0;
}

int w5500_udp_send(uint8_t socknum, uint8_t *ip, uint16_t port) {
    uint8_t stat;

    /* sanity check */
    if (socknum > W5500_MAX_SOCKETS || !ip) {
        return -EINVAL;
    }

    /* make sure we're in the right state */
    if (_socktable[socknum].state != S_ESTAB) {
        printf_P(PSTR("w5500: read from closed socket\r\n"));
        return -EIO;
    }

    /* check to see if the socket has been closed on us */
    if (_read_reg(BLK_SOCKET_REG(socknum),SOCK_SR) != SOCK_SR_UDP) {
        /* close it */
        printf_P(PSTR("w5500: read from closed socket\r\n"));
        return -EIO;
    }
    /* set dest details */
    _write_block(BLK_SOCKET_REG(socknum),SOCK_DIPR0,4,ip);
    _write_reg16(BLK_SOCKET_REG(socknum),SOCK_DPORT0,port);
    /* issue send */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_CR,SOCK_CR_SEND);

    /* wait for complete */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,SOCK_IR_SENDOK | SOCK_IR_TIMEOUT);

    while (!((stat = _read_reg(BLK_SOCKET_REG(socknum),SOCK_IR)) & (SOCK_IR_SENDOK | SOCK_IR_TIMEOUT)));
    /* clear interupt when done */
    _write_reg(BLK_SOCKET_REG(socknum),SOCK_IR,SOCK_IR_SENDOK | SOCK_IR_TIMEOUT);
    /* work out why we exited */
    if (stat & SOCK_IR_TIMEOUT) {
        printf_P(PSTR("w5500: udp timeout\r\n"));
        /* we don't close it, we just allow you to make that decision yourself */
        return -EIO;
    }
    _socktable[socknum].btxlen = 0;

    return 0;
}

uint16_t _find_free_port(void) {
    static uint16_t last_used = 0;
    uint8_t n;

    /* The W5500 datasheet does not make clear if the whole connection
     * set (src,sport,dest,dport) is used for port uniqueness, or
     * just the sport on our end. We've assumed sport must be unique
     * across all connections. */

    /* this assumes random number gen has been seeded */
    if (last_used == 0) {
        last_used = random()+1024;
    }

    last_used++; /* just to ensure some rotation */

    /* increment the port until we don't hit a used port */
    for (n = 0; n < W5500_MAX_SOCKETS; n++) {
        if (_socktable[n].state != S_CLOSED &&
            _socktable[n].port == last_used) {
            last_used++;
            if (last_used == 0) { /* wrap properly */
                last_used = 1024;
            }
        }
    }
    /* last_used should be unused by anything? */
    return last_used;
}
