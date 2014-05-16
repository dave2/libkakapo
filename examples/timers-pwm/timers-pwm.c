/* Pulse Width Modulation example */

/* This example sets up the timer 3, which is TCE0, to gently fade 
 * the yellow LED on PE3 in and out. As comp changes, the timer will
 * adjust how long during each cycle it will set the LED on. The
 * timer is responsible for actually driving the pin in this mode. 
 * As this cycles very quickly, the LED will appear to have different
 * brightness levels. */

#define F_CPU 32000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "timer.h"
#include "clock.h"

/* our main code */

int main(void) {
 uint16_t comp;
 uint8_t dir;

 /* ensure we're running at the expected clock rate, interrupts enabled */
 sysclk_init();

 /* set up timer 3 (TCE0) */
 /* for this, we just want 1024 levels of brightness */
 timer_init(3,timer_pwm,1023,NULL,NULL); 
 timer_clk(3,timer_perdiv1); /* start it running */

 PORTE.DIRSET = (PIN2_bm | PIN3_bm); /* make LEDs be LEDs */

 /* reset compare/dir values */
 comp = 1;
 dir = 1;

 /* now, loop looking to see if millis as hit 0 */
 while (1) {
  if (comp > 1022 || comp < 1) {
   dir = 1 - dir;
   PORTE.OUTTGL = PIN2_bm;
  }
  if (dir) {
   comp++;
  } else {
   comp--;
  }
  timer_comp_val(3,timer_ch_d,comp);
  _delay_ms(2); /* slow the rate at which comp changes */
 }

 /* never reached */
 return 0;
}
