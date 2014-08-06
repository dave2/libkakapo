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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "global.h"
#include "rtc.h"
#include "errors.h"

/* pointers to the compare/ovf hooks */
void (*cmp_fn)(void) = NULL;
void (*ovf_fn)(void) = NULL;

/* FIXME: unsure if the syncbusy checking is in the right places */

/* init the RTC */
int rtc_init(uint16_t period, void (*cmp_hook)(void), void (*ovf_hook)(void)) {
    RTC.PER = period;
    RTC.CNT = 0;
    /* ensure it's all written before we exit */
    while (RTC.STATUS & RTC_SYNCBUSY_bm);

    /* enable interupts for RTC */
    cmp_fn = cmp_hook;
    ovf_fn = ovf_hook;
    if (cmp_hook) {
        RTC.INTCTRL |= RTC_COMPINTLVL_LO_gc;
    }
    if (ovf_hook) {
        RTC.INTCTRL |= RTC_OVFINTLVL_LO_gc;
    }

    return 0;
}

/* set the divisor (which, if non-zero, runs it! */
int rtc_div(rtc_clk_div_t div) {
    if (div > rtc_div1024) {
        return -EINVAL;
    }
    RTC.CTRL = div; /* runs it if non-zero */
    /* wait until sync is done */
    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    return 0;
}

/* set the compare value */
int rtc_comp(uint16_t value) {
    RTC.COMP = value;
    /* wait until sync is done */
    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    return 0;
}

/* set the current count */
int rtc_setcount(uint16_t value) {
    uint8_t save;
    /* save the div value */
    save = RTC.CTRL;
    RTC.CTRL = 0; /* stop the RTC */
    RTC.CNT = value;
    /* wait until sync is done */
    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    /* restore the state */
    RTC.CTRL = save;
    /* wait until sync is done */
    while (RTC.STATUS & RTC_SYNCBUSY_bm);
    return 0;
}

/* return the current value of the RTC */
uint16_t rtc_count(void) {
    while (RTC.STATUS & RTC_SYNCBUSY_bm); /* loop until sync is complete */
    return RTC.CNT;
}

/* interrupt handling */
ISR(RTC_OVF_vect) {
    if (ovf_fn) {
        (*ovf_fn)();
    }
}

ISR(RTC_COMP_vect) {
    if (cmp_fn) {
        (*cmp_fn)();
    }
}
