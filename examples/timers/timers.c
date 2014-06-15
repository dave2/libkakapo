/* This example demonstrates how to set up a timer to count milliseconds
 * and to maintain those in a variable. It blinks the green LED on PE2
 * for each second that passes. We're using the call-back on an overflow
 * interrupt to maintain the millisecond count. */

#define F_CPU 32000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "global.h"
#include "timer.h"
#include "clock.h"

volatile uint16_t millis;

/* this function maintains millis */
void timer_overflow_hook(void) {
 millis++;
 if (millis == 1000) {
  millis = 0;
 }
}

/* our main code */

int main(void) {
 /* ensure we're running at the expected clock rate, interrupts enabled */
 sysclk_init();

 /* set up timer */
 /* 32MHz -> perclk/1 -> 32000 ticks per ovf -> 1ms */
 timer_init(timer_c0,timer_norm,31999,NULL,&timer_overflow_hook);
 timer_clk(timer_c0,timer_perdiv1); /* start it running */

 PORTE.DIRSET = (PIN2_bm | PIN3_bm); /* make LEDs be LEDs */

 /* now, loop looking to see if millis as hit 0 */
 while (1) {
  uint16_t millis_copy;

  /* protect access to volatile with all interrupts being disabled */
  cli(); millis_copy = millis; sei();

  if (millis_copy < 100) {
   PORTE.OUTSET = PIN2_bm;
  } else {
   PORTE.OUTCLR = PIN2_bm;
  }
  _delay_ms(10); /* give everyone time to do things */
 }

 /* never reached */
 return 0;
}
