/* Copyright (C) 2013-2014 David Zanetti
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
 	void (*ovf_fn)(); /**< pointer to a top hook for whole timer */
} timer_t;

#define TIMER_MAX 4

#define TIM0_HW TCC0
#define TIM0_EV_OVF EVSYS_CHMUX_TCC0_OVF_gc
#define TIM0_EV_CMPA EVSYS_CHMUX_TCC0_CCA_gc

#define TIM1_HW TCC1
#define TIM1_EV_OVF EVSYS_CHMUX_TCC1_OVF_gc
#define TIM1_EV_CMPA EVSYS_CHMUX_TCC1_CCA_gc

#define TIM2_HW TCD0
#define TIM2_EV_OVF EVSYS_CHMUX_TCD0_OVF_gc
#define TIM2_EV_CMPA EVSYS_CHMUX_TCD0_CCA_gc

#define TIM3_HW TCE0
#define TIM3_EV_OVF EVSYS_CHMUX_TCE0_OVF_gc
#define TIM3_EV_CMPA EVSYS_CHMUX_TCE0_CCA_gc

/* work out how many channels we have on this hardware */
#ifdef EVSYS_CH4MUX
#define MAX_EVENT 8
#else
#define MAX_EVENT 4
#endif // EVSYS_CH4MUX

/* global timer constructs */
timer_t *timers[TIMER_MAX];

/* private functions */
void _timer_ovf_hook(uint8_t num);
void _timer_cmp_hook(uint8_t num, uint8_t ch);

/* implementation! */

/* init the struct, and some asic stuff about the timer */
uint8_t timer_init(uint8_t timernum, timer_pwm_t mode, uint16_t period,
					void (*cmp_hook)(uint8_t), void (*ovf_hook)()) {
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
			timers[timernum]->type = 0;
			timers[timernum]->hw.hw0 = &TIM0_HW;
			/* power it on too */
			PR.PRPC &= ~(PR_TC0_bm);
			break;
		case 1:
			/* timer1 shall be TCC1 */
			timers[timernum]->type = 1;
			timers[timernum]->hw.hw1 = &TIM1_HW;
			/* and power it on */
			PR.PRPC &= ~(PR_TC1_bm);
			break;
		case 2:
			/* timer2 shall be TCD0 */
			timers[timernum]->type = 0;
			timers[timernum]->hw.hw0 = &TIM2_HW;
			/* power it on */
			PR.PRPD &= ~(PR_TC0_bm);
			break;
		case 3:
			/* timer3 shall be TCE0 */
			timers[timernum]->type = 0;
			timers[timernum]->hw.hw0 = &TIM3_HW;
			/* power it on */
			PR.PRPE &= ~(PR_TC0_bm);
			break;
		default:
			free(timers[timernum]);
			return ENODEV;
	}

	/* install the hooks */
	timers[timernum]->cmp_fn = cmp_hook;
	timers[timernum]->ovf_fn = ovf_hook;

	/* now set the timer mode and period */
	switch (timers[timernum]->type) {
		case 0:
			timers[timernum]->hw.hw0->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw0->PER = period;
			if (ovf_hook) {
				/* we don't use ERR hook */
				timers[timernum]->hw.hw0->INTCTRLA = TC_OVFINTLVL_LO_gc;
			}
			break;
		case 1:
			timers[timernum]->hw.hw1->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw1->PER = period;
			if (ovf_hook) {
				/* we don't use ERR hook */
				timers[timernum]->hw.hw1->INTCTRLA = TC_OVFINTLVL_LO_gc;
			}
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
		case 0:
			timers[timernum]->hw.hw0->CTRLA = clk;
			break;
		case 1:
			timers[timernum]->hw.hw1->CTRLA = clk; /* cheating! */
			break;
		default:
			return EINVAL;
	}

	return 0;
}

/* fixme, hook for ovf and compare should really be in init() and not
 * here */

/* set up compares */
uint8_t timer_comp(uint8_t timernum, timer_chan_t ch, uint16_t value,
	uint8_t cmp_ev) {
	uint8_t ev_match;

	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* work out the event mux base for compare events */
	/* note: this is really cheating, and presumes compare event numbers
	 * will always be sequential. */
	switch (timernum) {
		case 0:
			ev_match = TIM0_EV_CMPA;
			break;
		case 1:
			ev_match = TIM1_EV_CMPA;
			break;
		case 2:
			ev_match = TIM2_EV_CMPA;
			break;
		case 3:
			ev_match = TIM3_EV_CMPA;
			break;
	}

	/* now configure the compare */
	switch (timers[timernum]->type) {
		case 0:
			switch (ch) {
				case timer_ch_a:
					timers[timernum]->hw.hw0->CCABUF = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCAEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&EVSYS_CH0MUX+cmp_ev) = (ev_match);
					}
					if (timers[timernum]->cmp_fn) {
						timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCAINTLVL_gm);
						timers[timernum]->hw.hw0->INTCTRLB |= TC_CCAINTLVL_LO_gc;
					}
					break;
				case timer_ch_b:
					timers[timernum]->hw.hw0->CCBBUF = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCBEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&(EVSYS.CH0MUX)+cmp_ev) = (ev_match+1);
					}
					if (timers[timernum]->cmp_fn) {
						timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCBINTLVL_gm);
						timers[timernum]->hw.hw0->INTCTRLB |= TC_CCBINTLVL_LO_gc;
					}
					break;
				case timer_ch_c:
					timers[timernum]->hw.hw0->CCABUF = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCCEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&EVSYS_CH0MUX+cmp_ev) = (ev_match+2);
					}
					if (timers[timernum]->cmp_fn) {
						timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCCINTLVL_gm);
						timers[timernum]->hw.hw0->INTCTRLB |= TC_CCCINTLVL_LO_gc;
					}
					break;
				case timer_ch_d:
					timers[timernum]->hw.hw0->CCABUF = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCDEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&EVSYS_CH0MUX+cmp_ev) = (ev_match+3);
					}
					if (timers[timernum]->cmp_fn) {
						timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCDINTLVL_gm);
						timers[timernum]->hw.hw0->INTCTRLB |= TC_CCDINTLVL_LO_gc;
					}
					break;
				default:
					return EINVAL;
			}
			break;
		case 1:
			switch (ch) {
				case timer_ch_a:
					timers[timernum]->hw.hw1->CCABUF = value;
					timers[timernum]->hw.hw1->CTRLB |= TC1_CCAEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&EVSYS_CH0MUX+cmp_ev) = (ev_match);
					}
					if (timers[timernum]->cmp_fn) {
						timers[timernum]->hw.hw1->INTCTRLB &= ~(TC0_CCAINTLVL_gm);
						timers[timernum]->hw.hw1->INTCTRLB |= TC_CCAINTLVL_LO_gc;
					}
					break;
				case timer_ch_b:
					timers[timernum]->hw.hw1->CCBBUF = value;
					timers[timernum]->hw.hw1->CTRLB |= TC1_CCBEN_bm;
					if (cmp_ev < MAX_EVENT) {
						*(&(EVSYS.CH0MUX)+cmp_ev) = (ev_match+1);
					}
					if (timers[timernum]->cmp_fn) {
						timers[timernum]->hw.hw1->INTCTRLB &= ~(TC0_CCBINTLVL_gm);
						timers[timernum]->hw.hw1->INTCTRLB |= TC_CCBINTLVL_LO_gc;
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
		case 0:
			switch (ch) {
			case timer_ch_a:
					timers[timernum]->hw.hw0->CCA = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCAEN_bm;
					break;
			case timer_ch_b:
					timers[timernum]->hw.hw0->CCB = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCBEN_bm;
					break;
			case timer_ch_c:
					timers[timernum]->hw.hw0->CCC = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCCEN_bm;
					break;
			case timer_ch_d:
					timers[timernum]->hw.hw0->CCD = value;
					timers[timernum]->hw.hw0->CTRLB |= TC0_CCDEN_bm;
					break;
			default:
					return EINVAL;
			}
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

uint8_t timer_comp_off(uint8_t timernum, timer_chan_t ch) {
	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the compare */
	switch (timers[timernum]->type) {
		case 0:
			switch (ch) {
			case timer_ch_a:
					timers[timernum]->hw.hw0->CTRLB &= ~(TC0_CCAEN_bm);
					timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCAINTLVL_gm);
					break;
			case timer_ch_b:
					timers[timernum]->hw.hw0->CTRLB &= ~(TC0_CCBEN_bm);
					timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCBINTLVL_gm);
					break;
			case timer_ch_c:
					timers[timernum]->hw.hw0->CTRLB &= ~(TC0_CCCEN_bm);
					timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCCINTLVL_gm);
					break;
			case timer_ch_d:
					timers[timernum]->hw.hw0->CTRLB &= ~(TC0_CCDEN_bm);
					timers[timernum]->hw.hw0->INTCTRLB &= ~(TC0_CCDINTLVL_gm);
					break;
			default:
					return EINVAL;
			}
		case 1:
			switch (ch) {
				case timer_ch_a:
					timers[timernum]->hw.hw1->CTRLB &= ~(TC1_CCAEN_bm);
					timers[timernum]->hw.hw1->INTCTRLB &= ~(TC1_CCAINTLVL_gm);
					break;
				case timer_ch_b:
					timers[timernum]->hw.hw1->CTRLB &= ~(TC1_CCBEN_bm);
					timers[timernum]->hw.hw1->INTCTRLB &= ~(TC1_CCBINTLVL_gm);
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
/* fixme: move this to init */
uint8_t timer_ovf(uint8_t timernum, uint8_t ovf_ev) {

	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	if (ovf_ev >= MAX_EVENT && ovf_ev != -1) {
		return EINVAL;
	}


	/* fixme: remove event channel when passed -1 */
	/* and the event handler */
	if (ovf_ev != -1) {
		switch (timernum) {
			case 0:
				*(&EVSYS_CH0MUX+ovf_ev) = TIM0_EV_OVF;
				break;
			case 1:
				*(&EVSYS_CH0MUX+ovf_ev) = TIM1_EV_OVF;
				break;
			case 2:
				*(&EVSYS_CH0MUX+ovf_ev) = TIM2_EV_OVF;
				break;
			case 3:
				*(&EVSYS_CH0MUX+ovf_ev) = TIM3_EV_OVF;
				break;
			default:
				return ENODEV;
		}
	}

	return 0;
}

/* set the current count */
uint8_t timer_count(uint8_t timernum, uint16_t value) {
	if (timernum >= TIMER_MAX || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the count */
	switch (timers[timernum]->type) {
		case 0:
			timers[timernum]->hw.hw0->CNT = value;
			break;
		case 1:
			timers[timernum]->hw.hw1->CNT = value;
			break;
		default:
			return EINVAL;
	}

	return 0;
}

/* ovf handler */
void _timer_ovf_hook(uint8_t num) {
	if (timers[num] && timers[num]->ovf_fn) {
		(*(timers[num]->ovf_fn))(NULL);
	}
}

/* interrupts for OVF results */
ISR(TCC0_OVF_vect) { _timer_ovf_hook(0); }
ISR(TCC1_OVF_vect) { _timer_ovf_hook(1); }
ISR(TCD0_OVF_vect) { _timer_ovf_hook(2); }
ISR(TCE0_OVF_vect) { _timer_ovf_hook(3); }

/* cmp handler */
void _timer_cmp_hook(uint8_t num, uint8_t ch) {
	if (timers[num] && timers[num]->cmp_fn) {
		(*(timers[num]->cmp_fn))(ch);
	}
}

/* interrupts for CMP results */
ISR(TCC0_CCA_vect) { _timer_cmp_hook(0,0); }
ISR(TCC0_CCB_vect) { _timer_cmp_hook(0,1); }
ISR(TCC0_CCC_vect) { _timer_cmp_hook(0,2); }
ISR(TCC0_CCD_vect) { _timer_cmp_hook(0,3); }
ISR(TCC1_CCA_vect) { _timer_cmp_hook(1,0); }
ISR(TCC1_CCB_vect) { _timer_cmp_hook(1,1); }
ISR(TCD0_CCA_vect) { _timer_cmp_hook(2,0); }
ISR(TCD0_CCB_vect) { _timer_cmp_hook(2,1); }
ISR(TCD0_CCC_vect) { _timer_cmp_hook(2,2); }
ISR(TCD0_CCD_vect) { _timer_cmp_hook(2,3); }
ISR(TCE0_CCA_vect) { _timer_cmp_hook(3,0); }
ISR(TCE0_CCB_vect) { _timer_cmp_hook(3,1); }
ISR(TCE0_CCC_vect) { _timer_cmp_hook(3,2); }
ISR(TCE0_CCD_vect) { _timer_cmp_hook(3,3); }

