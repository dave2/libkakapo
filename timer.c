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
 	uint8_t type; /**< Which type (0,1,2,4,5) of timer is this? */
 	union { /* as a union so it only stores one pointer */
#ifdef _HAVE_TIMER_TYPE0
 		TC0_t *hw0; /**< HW pointer for type 0 */
#endif
#ifdef _HAVE_TIMER_TYPE1
 		TC1_t *hw1; /**< HW pointer for type 1 */
#endif
#ifdef _HAVE_TIMER_TYPE2
        TC2_t *hw2; /**< HW pointer for type 2 */
#endif
#ifdef _HAVE_TIMER_TYPE4
        TC4_t *hw4; /**< HW pointer for type 4 */
#endif
#ifdef _HAVE_TIMER_TYPE5
        TC5_t *hw5; /**< HW pointer for type 5 */
#endif
 	} hw;
 	void (*cmp_fn)(uint8_t); /**< pointer to a compare hook, for channel n */
 	void (*ovf_fn)(); /**< pointer to a top hook for whole timer */
} timer_t;

/* work out how many channels we have on this hardware */
#ifdef EVSYS_CH4MUX
#define MAX_EVENT 8
#else
#define MAX_EVENT 4
#endif // EVSYS_CH4MUX

/* global timer constructs */
timer_t *timers[MAX_TIMERS] = TIMERS_INIT;

/* private functions */
void _timer_ovf_hook(uint8_t num);
void _timer_cmp_hook(uint8_t num, uint8_t ch);

/* implementation! */

/* init the struct, and some asic stuff about the timer */
uint8_t timer_init(uint8_t timernum, timer_pwm_t mode, uint16_t period,
					void (*cmp_hook)(uint8_t), void (*ovf_hook)()) {
	if (timernum >= MAX_TIMERS) {
		return ENODEV;
	}
	/* create the timer struct and allocate it a type */
	timers[timernum] = malloc(sizeof(timer_t));

	if (!timers[timernum]) {
		return ENOMEM;
	}

	/* hardware map starts here */
	/* Note: TCD4, TCE4, TCE5, TCF4, and TCF5 don't currently exist
	 * on any hardware, but defined here in case they appear */
	switch (timernum) {
#ifdef TCC0
        case timer_c0:
            timers[timernum]->type = 0;
            timers[timernum]->hw.hw0 = &TCC0;
            PR.PRPC &= ~(PR_TC0_bm);
            break;
#endif // TCC0
#ifdef TCC1
        case timer_c1:
            timers[timernum]->type = 1;
            timers[timernum]->hw.hw1 = &TCC1;
            PR.PRPC &= ~(PR_TC1_bm);
            break;
#endif // TCC1
#ifdef TCC2
        case timer_c2:
            timers[timernum]->type = 2;
            timers[timernum]->hw.hw2 = &TCC2;
            /* fixme: er, what's the power control for type 2 timers? */
            //PR.PRPC &= ~(PR_TC2_bm);
            break;
#endif // TCC2
#ifdef TCC4
        case timer_c4:
            timers[timernum]->type = 4;
            timers[timernum]->hw.hw4 = &TCC4;
            PR.PRPC &= ~(PR_TC4_bm);
            break;
#endif // TCC4
#ifdef TCC5
        case timer_c5:
            timers[timernum]->type = 5;
            timers[timernum]->hw.hw5 = &TCC5;
            PR.PRPC &= ~(PR_TC5_bm);
            break;
#endif // TCC5
#ifdef TCD0
        case timer_d0:
            timers[timernum]->type = 0;
            timers[timernum]->hw.hw0 = &TCD0;
            PR.PRPD &= ~(PR_TC0_bm);
            break;
#endif // TCD0
#ifdef TCD1
        case timer_d1:
            timers[timernum]->type = 1;
            timers[timernum]->hw.hw1 = &TCD1
            PR.PRPD &= ~(PR_TC1_bm);
            break;
#endif // TCD1
#ifdef TCD2
        case timer_d2:
            timers[timernum]->type = 2;
            timers[timernum]->hw.hw2 = &TCD2;
            //PR.PRPD &= ~(PR_TC2_bm);
            break;
#endif // TCD2
#ifdef TCD4
        case timer_d4:
            timers[timernum]->type = 4;
            timers[timernum]->hw.hw4 = &TCD4;
            PR.PRPD &= ~(PR_TC4_bm);
            break;
#endif // TCD4
#ifdef TCD5
        case timer_d5:
            timers[timernum]->type = 5;
            timers[timernum]->hw.hw5 = &TCD5;
            PR.PRPD &= ~(PR_TC5_bm);
            break;
#endif // TCD5
#ifdef TCE0
        case timer_e0:
            timers[timernum]->type = 0;
            timers[timernum]->hw.hw0 = &TCE0;
            PR.PRPE &= ~(PR_TC0_bm);
            break;
#endif // TCE0
#ifdef TCE1
        case timer_e1:
            timers[timernum]->type = 1;
            timers[timernum]->hw.hw1 = &TCE1
            PR.PRPE &= ~(PR_TC1_bm);
            break;
#endif // TCE1
#ifdef TCE2
        case timer_e2:
            timers[timernum]->type = 2;
            timers[timernum]->hw.hw2 = &TCE2;
            //PR.PRPE &= ~(PR_TC2_bm);
            break;
#endif // TCD2
#ifdef TCE4
        case timer_e4:
            timers[timernum]->type = 4;
            timers[timernum]->hw.hw4 = &TCE4;
            PR.PRPE &= ~(PR_TC4_bm);
            break;
#endif // TCE4
#ifdef TCE5
        case timer_e5:
            timers[timernum]->type = 5;
            timers[timernum]->hw.hw5 = &TCE5;
            PR.PRPE &= ~(PR_TC5_bm);
            break;
#endif // TCD5
#ifdef TCF0
        case timer_f0:
            timers[timernum]->type = 0;
            timers[timernum]->hw.hw0 = &TCF0;
            PR.PRPF &= ~(PR_TC0_bm);
            break;
#endif // TCF0
#ifdef TCF1
        case timer_f1:
            timers[timernum]->type = 1;
            timers[timernum]->hw.hw1 = &TCF1
            PR.PRPF &= ~(PR_TC1_bm);
            break;
#endif // TCF1
#ifdef TCF2
        case timer_f2:
            timers[timernum]->type = 2;
            timers[timernum]->hw.hw2 = &TCF2;
            //PR.PRPF &= ~(PR_TC2_bm);
            break;
#endif // TCF2
#ifdef TCF4
        case timer_f4:
            timers[timernum]->type = 4;
            timers[timernum]->hw.hw4 = &TCF4;
            PR.PRPF &= ~(PR_TC4_bm);
            break;
#endif // TCF4
#ifdef TCF5
        case timer_f5:
            timers[timernum]->type = 5;
            timers[timernum]->hw.hw5 = &TCF5;
            PR.PRPF &= ~(PR_TC5_bm);
            break;
#endif // TCD5
		default:
			free(timers[timernum]);
			return ENODEV;
	}

	/* install the hooks */
	timers[timernum]->cmp_fn = cmp_hook;
	timers[timernum]->ovf_fn = ovf_hook;

	/* now set the timer mode and period */
	switch (timers[timernum]->type) {
#ifdef _HAVE_TIMER_TYPE0
		case 0:
			timers[timernum]->hw.hw0->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw0->PER = period;
			if (ovf_hook) {
				/* we don't use ERR hook */
				timers[timernum]->hw.hw0->INTCTRLA = TC_OVFINTLVL_LO_gc;
			}
			break;
#endif // _HAVE_TIMER_TYPE0
#ifdef _HAVE_TIMER_TYPE1
		case 1:
			timers[timernum]->hw.hw1->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw1->PER = period;
			if (ovf_hook) {
				/* we don't use ERR hook */
				timers[timernum]->hw.hw1->INTCTRLA = TC_OVFINTLVL_LO_gc;
			}
			break;
#endif // _HAVE_TIMER_TYPE1
        /* fixme: type 2 timers */
#ifdef _HAVE_TIMER_TYPE4
        case 4:
        	timers[timernum]->hw.hw4->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw4->PER = period;
			if (ovf_hook) {
				/* we don't use ERR hook */
				timers[timernum]->hw.hw4->INTCTRLA = TC_OVFINTLVL_LO_gc;
			}
#endif // _HAVE_TIMER_TYPE4
#ifdef _HAVE_TIMER_TYPE5
        case 5:
        	timers[timernum]->hw.hw5->CTRLB = mode; /* cheating */
			timers[timernum]->hw.hw5->PER = period;
			if (ovf_hook) {
				/* we don't use ERR hook */
				timers[timernum]->hw.hw5->INTCTRLA = TC_OVFINTLVL_LO_gc;
			}
#endif // HAVE_TIME_TYPE5
		default:
			free(timers[timernum]);
			return EINVAL;
	}

	return 0;
}

/* Clock source setting */
uint8_t timer_clk(uint8_t timernum, timer_clk_src_t clk) {
	if (timernum >= MAX_TIMERS || !timers[timernum]) {
		return ENODEV;
	}

	/* apply the clock source selection to the timer */
	switch (timers[timernum]->type) {
#ifdef _HAVE_TIMER_TYPE0
		case 0:
			timers[timernum]->hw.hw0->CTRLA = clk;
			break;
#endif // _HAVE_TIMER_TYPE0
#ifdef _HAVE_TIMER_TYPE1
		case 1:
			timers[timernum]->hw.hw1->CTRLA = clk; /* cheating! */
			break;
#endif // _HAVE_TIMER_TYPE1
        /* fixme: type 2 timers */
#ifdef _HAVE_TIMER_TYPE4
        case 4:
        	timers[timernum]->hw.hw4->CTRLA = clk; /* cheating! */
			break;
#endif // _HAVE_TIMER_TYPE4
#ifdef _HAVE_TIMER_TYPE5
        case 5:
        	timers[timernum]->hw.hw5->CTRLA = clk; /* cheating! */
			break;
#endif // _HAVE_TIMER_TYPE5
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

	if (timernum >= MAX_TIMERS || !timers[timernum]) {
		return ENODEV;
	}

	/* work out the event mux base for compare events */
	/* note: this is really cheating, and presumes compare event numbers
	 * will always be sequential. current ev system seems to do this anyway */
	switch (timernum) {
#ifdef TCC0
		case timer_c0:
			ev_match = EVSYS_CHMUX_TCC0_CCA_gc;
			break;
#endif // TCC0
#ifdef TCC1
        case timer_c1:
            ev_match = EVSYS_CHMUX_TCC1_CCA_gc;
            break;
#endif // TCC1
#ifdef TCC4
        case timer_c4:
            ev_match = EVSYS_CHMUX_TCC4_CCA_gc;
            break;
#endif // TCC4
#ifdef TCC5
        case timer_c5:
            ev_match = EVSYS_CHMUX_TCC5_CCA_gc;
            break;
#endif // TCC5
#ifdef TCD0
        case timer_d0:
            ev_match = EVSYS_CHMUX_TCD0_CCA_gc;
            break;
#endif // TCD0
#ifdef TCD1
        case timer_d1:
            ev_match = EVSYS_CHMUX_TCD1_CCA_gc;
            break;
#endif // TCD1
#ifdef TCD4
        case timer_d4:
            ev_match = EVSYS_CHMUX_TCD4_CCA_gc;
            break;
#endif // TCD4
#ifdef TCD5
        case timer_d5:
            ev_match = EVSYS_CHMUX_TCD5_CCA_gc;
            break;
#endif // TCD5
#ifdef TCE0
        case timer_e0:
            ev_match = EVSYS_CHMUX_TCE0_CCA_gc;
            break;
#endif // TCE0
#ifdef TCE1
        case timer_e1:
            ev_match = EVSYS_CHMUX_TCE1_CCA_gc;
            break;
#endif // TCE1
#ifdef TCE4
        case timer_e4:
            ev_match = EVSYS_CHMUX_TCE4_CCA_gc;
            break;
#endif // TCE4
#ifdef TCE5
        case timer_e5:
            ev_match = EVSYS_CHMUX_TCE5_CCA_gc;
            break;
#endif // TCE5
#ifdef TCF0
        case timer_f0:
            ev_match = EVSYS_CHMUX_TCF0_CCA_gc;
            break;
#endif // TCF0
#ifdef TCF1
        case timer_f1:
            ev_match = EVSYS_CHMUX_TCF1_CCA_gc;
            break;
#endif // TCF1
#ifdef TCF4
        case timer_f4:
            ev_match = EVSYS_CHMUX_TCF4_CCA_gc;
            break;
#endif // TCF4
#ifdef TCF5
        case timer_f5:
            ev_match = EVSYS_CHMUX_TCF5_CCA_gc;
            break;
#endif // TCF5
        default:
            ev_match = 0; /* so warnings are silenced */
	}

	/* now configure the compare */
	switch (timers[timernum]->type) {
#ifdef _HAVE_TIMER_TYPE0
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
#endif // _HAVE_TIMER_TYPE0
#ifdef _HAVE_TIMER_TYPE1
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
#endif // _HAVE_TIMER_TYPE1
        /* fixme: type 2 need some special code I think */
        /* fixme: type 4 and 5 timers */
		default:
			return EINVAL;
	}

	return 0;
}

/* change just the compare value */
uint8_t timer_comp_val(uint8_t timernum, timer_chan_t ch, uint16_t value) {

	if (timernum >= MAX_TIMERS || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the compare */
	switch (timers[timernum]->type) {
#ifdef _HAVE_TIMER_TYPE0
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
#endif // _HAVE_TIMER_TYPE0
#ifdef _HAVE_TIMER_TYPE1
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
#endif // _HAVE_TIMER_TYPE1
        /* fixme: type 2, 4, and 5 timers */
		default:
			return EINVAL;
	}

	return 0;
}

uint8_t timer_comp_off(uint8_t timernum, timer_chan_t ch) {
	if (timernum >= MAX_TIMERS || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the compare */
	switch (timers[timernum]->type) {
#ifdef _HAVE_TIMER_TYPE0
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
#endif // _HAVE_TIMER_TYPE0
#ifdef _HAVE_TIMER_TYPE1
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
#endif // _HAVE_TIMER_TYPE1
        /* fixme: type 2, 4 and 5 timers */
		default:
			return EINVAL;
	}

	return 0;
}

/* set up overflows */
/* fixme: move this to init */
uint8_t timer_ovf(uint8_t timernum, uint8_t ovf_ev) {

	if (timernum >= MAX_TIMERS || !timers[timernum]) {
		return ENODEV;
	}

	if (ovf_ev >= MAX_EVENT && ovf_ev != -1) {
		return EINVAL;
	}

	/* fixme: remove event channel when passed -1 */
	/* and the event handler */
	if (ovf_ev != -1) {
		switch (timernum) {
#ifdef TCC0
            case timer_c0:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCC0_OVF_gc;
                break;
#endif // TCC0
#ifdef TCC1
            case timer_c1:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCC1_OVF_gc;
                break;
#endif // TCC1
#ifdef TCC4
            case timer_c4:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCC4_OVF_gc;
                break;
#endif // TCC4
#ifdef TCC5
            case timer_c5:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCC5_OVF_gc;
                break;
#endif // TC50
#ifdef TCD0
            case timer_d0:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCD0_OVF_gc;
                break;
#endif // TCD0
#ifdef TCD1
            case timer_d1:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCD1_OVF_gc;
                break;
#endif // TCD1
#ifdef TCD4
            case timer_d4:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCD4_OVF_gc;
                break;
#endif // TCD4
#ifdef TCD5
            case timer_d5:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCD5_OVF_gc;
                break;
#endif // TCD5
#ifdef TCE0
            case timer_e0:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCE0_OVF_gc;
                break;
#endif // TCE0
#ifdef TCE1
            case timer_e1:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCE1_OVF_gc;
                break;
#endif // TCE1
#ifdef TCE4
            case timer_e4:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCE4_OVF_gc;
                break;
#endif // TCE4
#ifdef TCE5
            case timer_e5:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCE5_OVF_gc;
                break;
#endif // TCE5
#ifdef TCF0
            case timer_f0:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCF0_OVF_gc;
                break;
#endif // TCF0
#ifdef TCF1
            case timer_f1:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCF1_OVF_gc;
                break;
#endif // TCF1
#ifdef TCF4
            case timer_f4:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCF4_OVF_gc;
                break;
#endif // TCF4
#ifdef TCF5
            case timer_f5:
                *(&EVSYS_CH0MUX+ovf_ev) = EVSYS_CHMUX_TCF5_OVF_gc;
                break;
#endif // TCF5
			default:
				return ENODEV;
		}
	}

	return 0;
}

/* set the current count */
uint8_t timer_count(uint8_t timernum, uint16_t value) {
	if (timernum >= MAX_TIMERS || !timers[timernum]) {
		return ENODEV;
	}

	/* now configure the count */
	switch (timers[timernum]->type) {
#ifdef _HAVE_TIMER_TYPE0
		case 0:
			timers[timernum]->hw.hw0->CNT = value;
			break;
#endif // _HAVE_TIMER_TYPE0
#ifdef _HAVE_TIMER_TYPE1
		case 1:
			timers[timernum]->hw.hw1->CNT = value;
			break;
#endif // _HAVE_TIMER_TYPE1
        /* fixme: type 2 */
#ifdef _HAVE_TIMER_TYPE4
        case 4:
			timers[timernum]->hw.hw4->CNT = value;
			break;
#endif // _HAVE_TIMER_TYPE4
#ifdef _HAVE_TIMER_TYPE5
        case 5:
			timers[timernum]->hw.hw5->CNT = value;
			break;
#endif // _HAVE_TIMER_TYPE5
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
#ifdef TCC0
ISR(TCC0_OVF_vect) { _timer_ovf_hook(timer_c0); }
#endif // TCC0
#ifdef TCC1
ISR(TCC1_OVF_vect) { _timer_ovf_hook(timer_c1); }
#endif // TCC1
#ifdef TCC4
ISR(TCC4_OVF_vect) { _timer_ovf_hook(timer_c4); }
#endif // TCC4
#ifdef TCC5
ISR(TCC5_OVF_vect) { _timer_ovf_hook(timer_c5); }
#endif // TCC5
#ifdef TCD0
ISR(TCD0_OVF_vect) { _timer_ovf_hook(timer_d0); }
#endif // TCD0
#ifdef TCD1
ISR(TCD1_OVF_vect) { _timer_ovf_hook(timer_d1); }
#endif // TCD1
#ifdef TCD4
ISR(TCD4_OVF_vect) { _timer_ovf_hook(timer_d4); }
#endif // TCD4
#ifdef TCD5
ISR(TCD5_OVF_vect) { _timer_ovf_hook(timer_d5); }
#endif // TCD5
#ifdef TCE0
ISR(TCE0_OVF_vect) { _timer_ovf_hook(timer_e0); }
#endif // TCE0
#ifdef TCE1
ISR(TCE1_OVF_vect) { _timer_ovf_hook(timer_e1); }
#endif // TCE1
#ifdef TCE4
ISR(TCE4_OVF_vect) { _timer_ovf_hook(timer_e4); }
#endif // TCE4
#ifdef TCE5
ISR(TCE5_OVF_vect) { _timer_ovf_hook(timer_e5); }
#endif // TCE5
#ifdef TCF0
ISR(TCF0_OVF_vect) { _timer_ovf_hook(timer_f0); }
#endif // TCF0
#ifdef TCF1
ISR(TCF1_OVF_vect) { _timer_ovf_hook(timer_f1); }
#endif // TCF1
#ifdef TCF4
ISR(TCF4_OVF_vect) { _timer_ovf_hook(timer_f4); }
#endif // TCF4
#ifdef TCF5
ISR(TCF5_OVF_vect) { _timer_ovf_hook(timer_f5); }
#endif // TCF5

/* cmp handler */
void _timer_cmp_hook(uint8_t num, uint8_t ch) {
	if (timers[num] && timers[num]->cmp_fn) {
		(*(timers[num]->cmp_fn))(ch);
	}
}

/* interrupts for CMP results */
#ifdef TCC0
ISR(TCC0_CCA_vect) { _timer_cmp_hook(timer_c0,0); }
ISR(TCC0_CCB_vect) { _timer_cmp_hook(timer_c0,1); }
ISR(TCC0_CCC_vect) { _timer_cmp_hook(timer_c0,2); }
ISR(TCC0_CCD_vect) { _timer_cmp_hook(timer_c0,3); }
#endif // TCC0
#ifdef TCC1
ISR(TCC1_CCA_vect) { _timer_cmp_hook(timer_c1,0); }
ISR(TCC1_CCB_vect) { _timer_cmp_hook(timer_c1,1); }
#endif // TCC1
#ifdef TCC4
ISR(TCC4_CCA_vect) { _timer_cmp_hook(timer_c4,0); }
ISR(TCC4_CCB_vect) { _timer_cmp_hook(timer_c4,1); }
ISR(TCC4_CCC_vect) { _timer_cmp_hook(timer_c4,2); }
ISR(TCC4_CCD_vect) { _timer_cmp_hook(timer_c4,3); }
#endif // TCC4
#ifdef TCC5
ISR(TCC5_CCA_vect) { _timer_cmp_hook(timer_c5,0); }
ISR(TCC5_CCB_vect) { _timer_cmp_hook(timer_c5,1); }
#endif // TCC1
#ifdef TCD0
ISR(TCD0_CCA_vect) { _timer_cmp_hook(timer_d0,0); }
ISR(TCD0_CCB_vect) { _timer_cmp_hook(timer_d0,1); }
ISR(TCD0_CCC_vect) { _timer_cmp_hook(timer_d0,2); }
ISR(TCD0_CCD_vect) { _timer_cmp_hook(timer_d0,3); }
#endif // TCD0
#ifdef TCD1
ISR(TCD1_CCA_vect) { _timer_cmp_hook(timer_d1,0); }
ISR(TCD1_CCB_vect) { _timer_cmp_hook(timer_d1,1); }
#endif // TCD1
#ifdef TCD4
ISR(TCD4_CCA_vect) { _timer_cmp_hook(timer_d4,0); }
ISR(TCD4_CCB_vect) { _timer_cmp_hook(timer_d4,1); }
ISR(TCD4_CCC_vect) { _timer_cmp_hook(timer_d4,2); }
ISR(TCD4_CCD_vect) { _timer_cmp_hook(timer_d4,3); }
#endif // TCD4
#ifdef TCD5
ISR(TCD5_CCA_vect) { _timer_cmp_hook(timer_d5,0); }
ISR(TCD5_CCB_vect) { _timer_cmp_hook(timer_d5,1); }
#endif // TCD1
#ifdef TCE0
ISR(TCE0_CCA_vect) { _timer_cmp_hook(timer_e0,0); }
ISR(TCE0_CCB_vect) { _timer_cmp_hook(timer_e0,1); }
ISR(TCE0_CCC_vect) { _timer_cmp_hook(timer_e0,2); }
ISR(TCE0_CCD_vect) { _timer_cmp_hook(timer_e0,3); }
#endif // TCE0
#ifdef TCE1
ISR(TCE1_CCA_vect) { _timer_cmp_hook(timer_e1,0); }
ISR(TCE1_CCB_vect) { _timer_cmp_hook(timer_e1,1); }
#endif // TCC1
#ifdef TCE4
ISR(TCE4_CCA_vect) { _timer_cmp_hook(timer_e4,0); }
ISR(TCE4_CCB_vect) { _timer_cmp_hook(timer_e4,1); }
ISR(TCE4_CCC_vect) { _timer_cmp_hook(timer_e4,2); }
ISR(TCE4_CCD_vect) { _timer_cmp_hook(timer_e4,3); }
#endif // TCC4
#ifdef TCE5
ISR(TCE5_CCA_vect) { _timer_cmp_hook(timer_e5,0); }
ISR(TCE5_CCB_vect) { _timer_cmp_hook(timer_e5,1); }
#endif // TCC1
#ifdef TCF0
ISR(TCF0_CCA_vect) { _timer_cmp_hook(timer_f0,0); }
ISR(TCF0_CCB_vect) { _timer_cmp_hook(timer_f0,1); }
ISR(TCF0_CCC_vect) { _timer_cmp_hook(timer_f0,2); }
ISR(TCF0_CCD_vect) { _timer_cmp_hook(timer_f0,3); }
#endif // TCF0
#ifdef TCF1
ISR(TCF1_CCA_vect) { _timer_cmp_hook(timer_f1,0); }
ISR(TCF1_CCB_vect) { _timer_cmp_hook(timer_f1,1); }
#endif // TCF1
#ifdef TCF4
ISR(TCF4_CCA_vect) { _timer_cmp_hook(timer_f4,0); }
ISR(TCF4_CCB_vect) { _timer_cmp_hook(timer_f4,1); }
ISR(TCF4_CCC_vect) { _timer_cmp_hook(timer_f4,2); }
ISR(TCF4_CCD_vect) { _timer_cmp_hook(timer_f4,3); }
#endif // TCF4
#ifdef TCF5
ISR(TCF5_CCA_vect) { _timer_cmp_hook(timer_f5,0); }
ISR(TCF5_CCB_vect) { _timer_cmp_hook(timer_f5,1); }
#endif // TCF1
