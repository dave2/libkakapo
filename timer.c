/* Copyright (C) 2013 David Zanetti
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* generic XMEGA timer driver */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdlib.h>

#include "global.h"
#include <util/delay.h>

#include "timer.h"
#include "errors.h"

typedef struct {
 	uint8_t type; /**< is this a type 0 or type 1 timer */
 	union { /* as a union so it only stores one pointer */
 		TC0_t *hw0; /**< HW pointer for type 0 */
 		TC1_t *hw1; /**< HW pointer for type 1 */
 	} hw;
 	void (*cmp_fn)(uint8_t); /**< pointer to a compare hook, for channel n */
 	void (*ovf_fn)(uint8_t); /**< pointer to a top hook for channel n */
} timer_t;

#define TIMER_MAX 2

#define TIM0_HW TCC1
#define TIM0_EV_OVF EVSYS_CHMUX_TCC1_OVF_gc
#define TIM0_EV_CMPA EVSYS_CHMUX_TCC1_CCA_gc
#define TIM0_EV_CMPB EVSYS_CHMUX_TCC1_CCB_gc

/* work out how many channels we have on this hardware */
#ifdef EVSYS_CH4MUX
#define MAX_EVENT 8
#else
#define MAX_EVENT 4
#endif // EVSYS_CH4MUX

/* global timer constructs */
timer_t *timers[TIMER_MAX];

/* implementation! */

/* init the struct, and some asic stuff about the timer */
uint8_t timer_init(uint8_t timernum, timer_pwm_t mode, uint16_t period) {
	if (timernum >= TIMER_MAX) {
		return ENODEV;
	}
	/* create the timer struct and allocate it a type */
	timers[timernum] = malloc(sizeof(timer_t));

	if (!timers[timernum]) {
		return ENOMEM;
	}

	/* map the timer numbers into hardware  */
	switch (timernum) {
		case 0:
			/* timer0 shall be TCC1 */
			timers[timernum]->type = 1;
			timers[timernum]->hw.hw1 = &TIM0_HW;
			/* and power it on */
			PR.PRPC &= ~(PR_TC1_bm);
			break;
		default:
			free(timers[timernum]);
			return ENODEV;
	}
	/* reset function pointers */
	timers[timernum]->cmp_fn = NULL;
	timers[timernum]->ovf_fn = NULL;

	/* now set the timer mode and period */
	switch (timers[timernum]->type) {
		case 1:
			timers[timernum]->hw.hw1->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw1->PER = period;
			break;
		default:
			free(timers[timernum]);
			return EINVAL;
	}

	return 0;
}

/* Clock source setting */
uint8_t timer_clk(uint8_t timernum, timer_clk_src_t clk) {
	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* apply the clock source selection to the timer */
	switch (timers[timernum]->type) {
		case 1:
			timers[timernum]->hw.hw1->CTRLA = clk; /* cheating! */
			break;
		default:
			return EINVAL;
	}

	return 0;
}

/* set up compares */
uint8_t timer_comp(uint8_t timernum, timer_chan_t ch, uint16_t value,
	void (*cmp_hook)(uint8_t), uint8_t cmp_ev) {

	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* install the hook */
	timers[timernum]->cmp_fn = cmp_hook;

	/* now configure the compare */
	switch (timers[timernum]->type) {
		case 1:
			switch (ch) {
				case timer_ch_a:
					timers[timernum]->hw.hw1->CCABUF = value;
					timers[timernum]->hw.hw1->CTRLB |= TC1_CCAEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&EVSYS_CH0MUX+cmp_ev) = TIM0_EV_CMPA;
					}
					break;
				case timer_ch_b:
					timers[timernum]->hw.hw1->CCBBUF = value;
					timers[timernum]->hw.hw1->CTRLB |= TC1_CCBEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&(EVSYS.CH0MUX)+cmp_ev) = TIM0_EV_CMPB;
					}
					break;
				default:
					return EINVAL;
			}
			break;
		default:
			return EINVAL;
	}

	return 0;
}

/* change just the compare value */
uint8_t timer_comp_val(uint8_t timernum, timer_chan_t ch, uint16_t value) {

	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the compare */
	switch (timers[timernum]->type) {
		case 1:
			switch (ch) {
				case timer_ch_a:
					timers[timernum]->hw.hw1->CCA = value;
					timers[timernum]->hw.hw1->CTRLB |= TC1_CCAEN_bm;
					break;
				case timer_ch_b:
					timers[timernum]->hw.hw1->CCB = value;
					timers[timernum]->hw.hw1->CTRLB |= TC1_CCBEN_bm;
					break;
				default:
					return EINVAL;
			}
			break;
		default:
			return EINVAL;
	}

	return 0;
}

/* set up overflows */
uint8_t timer_ovf(uint8_t timernum, void (*ovf_hook)(uint8_t),
	uint8_t ovf_ev) {

	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	if (ovf_ev >= MAX_EVENT) {
		return EINVAL;
	}

	/* install the hook */
	timers[timernum]->ovf_fn = ovf_hook;
	/* and the event handler */
	*(&EVSYS_CH0MUX+ovf_ev) = TIM0_EV_OVF;

	return 0;
}

/* set the current count */
uint8_t timer_count(uint8_t timernum, uint16_t value) {
	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the count */
	switch (timers[timernum]->type) {
		case 1:
			timers[timernum]->hw.hw1->CNT = value;
			break;
		default:
			return EINVAL;
	}

	return 0;
}
