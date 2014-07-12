/* RTC example */

/* This sets up the RTC to be clocked from the internal 32.768kHz
 * RC osc, overflowing once a second, which is used to maintain
 * a count of seconds passed. This is output to the usart connected
 * to USB. Overflow sets the LED, compare (set to 100ms roughly)
 * turns off the LED.
 *
 * Compare this to examples/timer which does a similar thing but
 * clocked from the system clock, rather than the internal
 * 32.768kHz RC OSC.
 */

#define F_CPU 32000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include "global.h"
#include "kakapo.h"
#include "rtc.h"
#include "clock.h"
#include "usart.h"

/* since it's being modified by an interrupt, mark it volatile */
volatile uint32_t uptime;

/* this runs in an interrupt context! */
void rtc_overflow_hook(void) {
 PORTE.OUTSET = PIN3_bm;
 uptime++;
}

void rtc_compare_hook(void) {
 PORTE.OUTCLR = PIN3_bm;
}

int main(void) {
    /* set up the board */
    kakapo_init();

    /* let's set up the usart connected to USB to report our uptime */
    /* configure the usart for 9600,8,N,1 */
    /* usart_d0 is attached to USB */
    usart_init(usart_d0, 128, 128); /* 128 byte RX and TX buffers */
    usart_conf(usart_d0, 9600, 8, none, 1, 0, NULL); /* 9600,8,N,1; no feat; no callback */
    usart_map_stdio(usart_d0); /* first allocated one is stdin/stdout/stderr */
    usart_run(usart_d0); /* get it going */

    /* we need to start the internal 32.768kHz oscilator before
     * it can be used by the RTC. */
    if (clock_osc_run(osc_rc32khz)) {
        printf("failed to start 32kHz RC OSC\r\n");
    }

    /* now we need to select it as the RTC source. In this case,
     * we're using it in 32.768kHz mode, instead of 1024Hz mode */
    if (clock_rtc(rtc_clk_rcosc32)) {
        printf("failed to select 32kHz RC OSC as RTC source\r\n");
    }

    /* now we can configure the RTC the way we want it */

    /* init the RTC. period is 32767 so 1Hz overflows, and just
     * the overflow hook */
    rtc_init(32767,&rtc_compare_hook,&rtc_overflow_hook);

    /* set a compare for about 3276 ticks in, roughly 100ms after overflow */
    rtc_comp(3276);

    /* now run it! */
    rtc_div(rtc_div1);

    while (1) {
        printf("%ld\r\n",uptime);
        _delay_ms(1000);
    }

    return 0; /* never reached */
}
