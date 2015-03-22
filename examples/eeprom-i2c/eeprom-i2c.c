/*
 * Example of how to use the libkakapo TWI/I2C functions
 * In this case, we're reading the MAC address from a Microchip
 * 24AA02E48T I2C EUI-48 eeprom.
 *
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include <util/delay.h>
#include "usart.h"
#include "kakapo.h"
#include "twi.h"
#include "clock.h"
#include "errors.h"

uint8_t mac[6]; /* where we place our MAC address from the chip */

int main(void) {
  kakapo_init();

  sei();

  // configure the usart_d0 (attached to USB) for 9600,8,N,1 to
  // show debugging info, see usart examples for explanation
  usart_init(usart_d0, 128, 128);
  usart_conf(usart_d0, 921600, 8, none, 1, 0, NULL);
  usart_map_stdio(usart_d0);
  usart_run(usart_d0);

  putchar(0);
  _delay_ms(1);

  /* initalise I2C to run at 400kHz, 200us timeout */
  twi_init(twi_e,400,200);

  /* the 24AA02E48T is at address 0x50 and needs a write of an address
   * to read from, then reads are successive bytes from that address
   * in the eeprom */
  /* in this example we do a multi-part read, to illustrate that read/write
   * may be done as multiple calls */
  if (twi_start(twi_e,0x50,twi_mode_write) == 0) {
    twi_write(twi_e,"\xFA",1,twi_more);
    twi_start(twi_e,0x50,twi_mode_read);
    twi_read(twi_e,&(mac),3,twi_more);
    twi_read(twi_e,&(mac[3]),3,twi_stop);
    printf_P(PSTR("\r\nMAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    printf_P(PSTR("no MAC chip at 0x50\r\n"));
  }

    while (1) {
        // do nothing
    }

  return 0;
}

