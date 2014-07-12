/* usart hello world example */

/* This is a simple demo of avr-libc based usart comms. Once a port is initialised
 * and mapped into a handle or stdio, you can use common libc functions
 * like printf() to interact with the serial port.
 */

#define F_CPU 32000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "usart.h"
#include "clock.h"

/* out main code */

int main(void) {
 /* ensure we're running at the expected clock rate */
 kakapo_init();
 sei();

 /* configure the usart for 9600,8,N,1 */
 /* usart_d0 is attached to USB */
 usart_init(usart_d0, 128, 128); /* 128 byte RX and TX buffers */
 usart_conf(usart_d0, 9600, 8, none, 1, 0, NULL); /* 9600,8,N,1; no feat; no callback */
 usart_map_stdio(usart_d0); /* first allocated one is stdin/stdout/stderr */
 usart_run(usart_d0); /* get it going */

 /* blink an LED for each time we write */
 PORTE.DIRSET = PIN3_bm;
 while (1) {
  printf("Hello, World!\r\n"); /* note trailing return/newline */
  PORTE.OUTTGL = PIN3_bm; /* toggle yellow LED */
  _delay_ms(1000);
 }

 /* never reached */
 return 0;
}
