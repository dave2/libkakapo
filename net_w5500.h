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

#ifndef NET_W5500_H_INCLUDED
#define NET_W5500_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  \brief W5500 driver public interface
 *
 *  An implementation of a driver for the WizNet W5500 chip.
 *
 *  WizNet W5500 implements a PHY, MAC, and IPv4 stack covering
 *  ICMP, TCP, UDP, and offers a RAW mode for custom protocol
 *  handling. Supports 8 sockets with a total of 32kB of SRAM.
 *
 *  Sockets are read/write over a continual wrapping 64kB window
 *  mapped into the appropriate buffer space. No need to track
 *  MSS/MTU.
 *
 *  Chip does NOT have a pre-programmed MAC, one must be supplied.
 *
 *  http://www.wiznet.co.kr/w5500
 */

/** \brief W5500 socket protocol/mode */
typedef enum {
    w5500_tcp, /**< Socket protocol is TCP */
    w5500_udp, /**< Socket protocol is UDP */
    w5500_macraw, /**< Socket "protocol" is MACRAW mode */
} w5500_sockmode_t;

/** \brief Initalise a W5500 chip
 *
 *  \param spi_port SPI port to initalise and use
 *  \param cs_port HW port where CS pin is
 *  \param cs_pin Pin to use
 *  \param mac[6] MAC address to initalise with
 *  \return 0 on success, errors.h otherwise
 */
int w5500_init(spi_portname_t spi_port, PORT_t *cs_port, uint8_t cs_pin,
		uint8_t *mac);

/** \brief W5500 IPv4 configuration
 *
 *  \param ip[4] IP Address
 *  \param mask_cidr Netmask in CIDR notation
 *  \param gw[4] Gateway IP Address
 *  \return 0 on success, errors.h otherwise
 */
int w5500_ip_conf(uint8_t *ip, uint8_t mask_cidr, uint8_t *gw);

/** \brief W5500 Socket initalisation
 *
 *  Sockets have a mode, and a size in kB for their tx and rx circular buffer.
 *  Note that the sum of RX buffers cannot exceed 16kB, same for TX buffers
 *
 *  \param socknum Socket number (from zero)
 *  \param rxsize Size of circular buffer for RX, must be pow 2 =<16kB
 *  \param txsize Size of circular buffer for TX, must be pow 2 =<16kB
 *  \return 0 on success, errors.h otherwise
 */
int w5500_socket_init(uint8_t socknum, uint16_t rxsize, uint16_t txsize);

/** \brief Establish TCP connection
 *
 *  The socket is prepared for TCP and connection to the address/port
 *  given is attempted. This will randomly select an unused source port
 *  on our end in the range 1024-65535
 *
 *  \param socknum Socket number (from zero)
 *  \param addr[4] IPv4 address
 *  \param port Destination port
 *  \return 0 on success, errors.h otherwise
 */
int w5500_tcp_connect(uint8_t socknum, uint8_t *addr, uint16_t port);

/** \brief Disconnect TCP connection
 *
 *  The socket is disconnected, gracefully where possible. The chip
 *  will automatically force to closed if a timeout expires.
 *
 *  Note: we don't report if the close was clean or not.
 *
 *  \param socknum Socket number (from zero)
 *  \return 0 on success, errors.h otherwise.
 */
int w5500_tcp_close(uint8_t socknum);

/** \brief Map a socket to a FILE stream.
 *
 *  This should be called on each physical socket ONLY ONCE. The
 *  code will handle the fact the socket is closed, so you don't
 *  need to keep creating a map for each new connection.
 *
 *  Call this after the socket has been prepared by
 *  w5500_socket_init(). You probably want to call this after
 *  usart init since otherwise it'll end up as stdin/stdout!
 *
 *  Note: this assumes TCP mode for the socket, which you may
 *  not have set yet.
 *
 *  \param socknum Socket number (from zero)
 *  \param bufsize Internal buffer size. Used to reduce overhead
 *  from commands over SPI.
 *  \return FILE pointer, or NULL if failed
 */
FILE *w5500_tcp_map_stdio(uint8_t socknum, uint16_t bufsize);

/** \brief Check to see if a socket has waiting RX
 *
 *  This is primarily so that rather than constantly polling gets()
 *  you can quickly check what is left to read.
 *
 *  Note that this returns the combination of whatever is buffered
 *  plus the chip RX buffer, since we read lumps of bufsize data
 *  from the chip.
 *
 *  \param socknum Socket number to check
 *  \return characters able to be read from the socket
 */
uint16_t w5500_tcp_unread(uint8_t socknum);

/** \brief Check to see how much space we have available in
 *  a socket to write into.
 *
 *  This includes characters already buffered inside the the stdio
 *  layer since we will want to commit those at some point.
 *
 *  \param socknum Socket number to check
 *  \return characters able to be written to the socket
 */
uint16_t w5500_tcp_writable(uint8_t socknum);

/** \brief Force a TCP socket to push data out.
 *
 *  This performs two functions. Firstly it flushes the
 *  internal tx buffer from stdio layer to the chip.
 *  It also forces the chip to send immediately, rather
 *  than wait for a full MSS sized packet.
 *
 *  \param socknum Socket number to push
 *  \return 0 on success, errors.h otherwise
 */
int w5500_tcp_push(uint8_t socknum);

/** \brief Listen on a UDP port.
 *
 *  UDP, regardless of direction, is always listen based on our end. We open
 *  a port to accept UDP packets or send packets from.
 *
 *  \param socknum Socket number to use (must have been init'd before)
 *  \param port Port on our end
 *  \return 0 on success, errors.h otherwise
 */
int w5500_udp_listen(uint8_t socknum, uint16_t port);

/** \brief Retrieve UDP packet metadata
 *
 *  Since UDP packets could come from anywhere into our socket, the chip
 *  prepends metadata (source ip, source port, and length) to the actual
 *  payload from the UDP packet.
 *
 *  Note that if you get -EIO from this, then it's very likely the UDP socket
 *  is broken in some way, and it'll be closed automatically.
 *
 *  \param socknum Socket number to use (must have been udp_listen before)
 *  \param ip[4] Buffer to write far IP address into (4 bytes)
 *  \param port Buffer to write far port into (2 bytes)
 *  \param len Buffer to write far paylong length into (2 bytes)
 *  \return 0 on success, errors.h otherwise
 */
int w5500_udp_rxmeta(uint8_t socknum, uint8_t *ip, uint16_t *port, uint16_t *len);

/** \brief Retrieve UDP payload
 *
 *  Copies len bytes into the buffer passed. If buf is NULL, this has the effect
 *  of discarding that number of bytes. If there are insufficient bytes, it will
 *  copy what is available and return that length.
 *
 *  Note: this MUST be called after the metadata is obtained, otherwise it has
 *  no idea what the length available is.
 *
 *  \param socknum Socket number to use
 *  \param len Length of bytes to read
 *  \param buf[] Pointer to buffer to write to (may be NULL)
 *  \return actual number of bytes provided
 */
int w5500_udp_read(uint8_t socknum, uint16_t len, uint8_t *buf);

/** \brief Close a UDP listener
 *
 *  Just closes the socket. This is not graceful since UDP isn't graceful. :)
 *
 *  \param socknum Socket number to close
 *  \return 0 on success, errors.h otherwise
 */
int w5500_udp_close(uint8_t socknum);

/** \brief Write to a UDP socket buffer
 *
 *  This writes out packet data but does not send it (you need udp_send for
 *  that.
 *
 *  \param socknum Socket number to use
 *  \param len Length of the data to buffer
 *  \param buf[] Pointer to the buffer to read from
 *  \return 0 on success, errors.h otherwise
 */
int w5500_udp_write(uint8_t socknum, uint16_t len, uint8_t *buf);

/** \brief Send the current UDP packet buffer
 *
 *  This performs the actual UDP packet send. It should only ever send
 *  one packet.
 *
 *  Note: on error, this DOES NOT close the socket. You have to clean up
 *  this state yourself.
 *
 *  \param socknum Socket number to use
 *  \param ip[4] IP address to send to
 *  \param port UDP port to send to
 *  \return 0 on success, errors.h otherwise.
 */
int w5500_udp_send(uint8_t socknum, uint8_t *ip, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif // NET_W5500_H_INCLUDED
