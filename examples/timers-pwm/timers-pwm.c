/* Pulse Width Modulation example */

/* This example sets up the timer 3, which is TCE0, to turn on an LED
 * for specific duration (duty) in a rapid cycle. This has the effect
 * of changing what brightness the LED appears to be. We make this
 * appear to fade in and out by adjusting the duty for the timer
 * channel attached to the LED. The timer is actually responsible for
 * switching the LED on and off as required.
 */

#define F_CPU 32000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "global.h"
#include "timer.h"
#include "kakapo.h"

/* our main code */

int main(void) {
 uint16_t comp;
 uint8_t dir;

 /* set up the development board (system clock, LEDs, etc) */
 kakapo_init();

 /* set up timer attached to yellow LED (TCE0, CH D) */
 /* for this, we just want 1024 levels of brightness */
 timer_init(timer_e0,timer_pwm,1023,NULL,NULL);
 timer_clk(timer_e0,timer_perdiv1); /* start it running */

 PORTE.DIRSET = (PIN2_bm | PIN3_bm); /* make LEDs be LEDs */

 /* reset compare/dir values */
 comp = 1;
 dir = 1;

 while (1) {
  /* bounce off the top and bottom, change direction */
  if (comp > 1022 || comp < 1) {
   dir = 1 - dir;
   PORTE.OUTTGL = PIN2_bm;
  }
  /* when going up, increase comp value (longer cycle), reverse for down */
  if (dir) {
   comp++;
  } else {
   comp--;
  }
  /* apply the new duty cycle to the channel */
  timer_comp_val(timer_e0,timer_ch_d,comp);
  _delay_ms(2); /* slow the rate at which comp changes */
 }

 /* never reached */
 return 0;
}
