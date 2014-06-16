#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

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

/* XMEGA timer public interface */

/** \brief An enum covering each platform's defined timers */
#if defined(_xmega_type_A1)

#define MAX_TIMERS 8
#define TIMERS_INIT {0,0,0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_d0,
    timer_d1,
    timer_e0,
    timer_e1,
    timer_f0,
    timer_f1,
} timer_portname_t;

#elif defined(_xmega_type_A1U)

#define MAX_TIMERS 12
#define TIMERS_INIT {0,0,0,0,0,0,0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_d0,
    timer_d1,
    timer_d2,
    timer_e0,
    timer_e1,
    timer_e2,
    timer_f0,
    timer_f1,
    timer_f2,
} timer_portname_t;

#elif defined(_xmega_type_A3) || defined(_xmega_type_256A3B) || defined(_xmega_type_A3U)

#define MAX_TIMERS 7
#define TIMERS_INIT {0,0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_d0,
    timer_d1,
    timer_e0,
    timer_e1,
    timer_f0,
} timer_portname_t;

#elif defined(_xmega_type_256A3BU)

#define MAX_TIMERS 11
#define TIMERS_INIT {0,0,0,0,0,0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_d0,
    timer_d1,
    timer_d2,
    timer_e0,
    timer_e1,
    timer_e2,
    timer_f0,
    timer_f2,
} timer_portname_t;

#elif defined(_xmega_type_A4) || defined(_xmega_type_D3)

#define MAX_TIMERS 5
#define TIMERS_INIT {0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_d0,
    timer_e0,
    timer_f0,
} timer_portname_t;

#elif defined(_xmega_type_A4U)

#define MAX_TIMERS 7
#define TIMERS_INIT {0,0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_d0,
    timer_d1,
    timer_d2,
    timer_e0
} timer_portname_t;

#elif defined (_xmega_type_B1)

#define MAX_TIMERS 5
#define TIMERS_INIT {0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_e0,
    timer_e2,
} timer_portname_t;

#elif defined (_xmega_type_B3)

#define MAX_TIMERS 3
#define TIMERS_INIT {0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
} timer_portname_t;

#elif defined(_xmega_type_C3)

#define MAX_TIMERS 9
#define TIMERS_INIT {0,0,0,0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_d0,
    timer_d2,
    timer_e0,
    timer_e2
    timer_f0,
    timer_f2,
} timer_portname_t;

#elif defined(_xmega_type_C4)

#define MAX_TIMERS 6
#define TIMERS_INIT {0,0,0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_d0,
    timer_d2,
    timer_e0,
} timer_portname_t;

#elif defined(_xmega_type_D4)

#define MAX_TIMERS 6
#define TIMERS_INIT {0,0,0,0}
#define _HAVE_TIMER_TYPE0
#define _HAVE_TIMER_TYPE1
#define _HAVE_TIMER_TYPE2

typedef enum {
    timer_c0 = 0,
    timer_c1,
    timer_c2,
    timer_d0,
    timer_d2,
    timer_e0,
} timer_portname_t;

#elif defined (_xmega_type_E5)

#define MAX_TIMERS 3
#define TIMERS_INIT {0,0,0}
#define _HAVE_TIMER_TYPE4
#define _HAVE_TIMER_TYPE5

typedef enum {
    timer_c4 = 0,
    timer_c5,
    timer_d5,
} timer_portname_t;

#endif

/** \brief Various clock sources for the timer
 *
 *  Split between CLKper with various divisors, and events on an
 *  event channel. */
typedef enum {
	timer_off = 0, /**< Stop running the timer */
	timer_perdiv1, /**< CLKper/1 */
	timer_perdiv2, /**< CLKper/2 */
	timer_perdiv4, /**< CLKper/4 */
	timer_perdiv8, /**< CLKper/8 */
	timer_perdiv64, /**< CLKper/64 */
	timer_perdiv256, /**< CLKper/256 */
	timer_perdiv1024, /**< CLKper/1024 */
	timer_ev0, /**< Event Channel 0 */
	timer_ev1, /**< Event Channel 1 */
	timer_ev2, /**< Event Channel 2 */
	timer_ev3, /**< Event Channel 3 */
	timer_ev4, /**< Event Channel 4 */
	timer_ev5, /**< Event Channel 5 */
	timer_ev6, /**< EVent Channel 6 */
	timer_ev7, /**< Event Channel 7 */
} timer_clk_src_t;

/** \brief Timer PWM modes */
typedef enum {
	timer_norm, /**< Normal 0->PER, no outputs on compare */
	timer_freq, /**< Frequency generation */
	timer_reserved1, /**< Unused */
	timer_pwm, /**< PWM: Single Slope */
	timer_reserved2, /**< Unused */
	timer_pwm_dstop, /**< PWM: Dual Slope, OVF/Event at top */
	timer_pwm_dsboth, /**< PWM: Dual Slope, OVF/Event at both */
	timer_pwm_dsbot, /**< PWM: Dual Slope, OVF/Event at bottom */
} timer_pwm_t;

/** \brief Timer channels */
typedef enum {
	timer_ch_a = 0, /**< Channel A */
	timer_ch_b, /**< Channel B */
	timer_ch_c, /**< Channel C, Type 0,2,4 only */
	timer_ch_d, /**< Channel D, Type 0,2,4 only */
} timer_chan_t;

/** \brief Initialise the given timer slot
 *
 *  The cmp_hook function must return void, and accept
 *  a single uint8_t argument being the channel number of the compare
 *  event (0=A). Can be NULL if unused.
 *
 *  The ovf_hook function must return void and accept no arguments.
 *  May be provided as NULL if not required.
 *
 *  \param timernum Number of the timer
 *  \param mode Timer mode (normal, pwm, etc)
 *  \param period Timer period (ie, TOP value)
 *  \param cmp_hook Function to invoke on compare matches
 *  \param ovf_hook Function to invoke on overflow
 *  \return 0 on success, errors.h otherwise */
uint8_t timer_init(timer_portname_t timer, timer_pwm_t mode, uint16_t period,
				void (*cmp_hook)(uint8_t), void (*ovf_hook)());

/** \brief Clock the timer from the given source
 *
 *  Note: this starts the timer running for any value other than
 *  timer_off.
 *
 *  \param timernum Number of the timer
 *  \param clk Clock source/divider
 *  \return 0 on success, errors.h otherwise.  */
uint8_t timer_clk(timer_portname_t timer, timer_clk_src_t clk);

/** \brief Set the given channel compare
 *
 *  The cmp_hook function must return void, and accept
 *  a single uint8_t argument being the channel number of the compare
 *  event (0=A). They can be NULL if unused.
 *
 *  Note: In PWM modes, compares also toggle their associated pins
 *  automatically, if set as output.
 *
 *  Note: This uses the compare buffer, so is safe to do during timer
 *  running.
 *
 *  \param timernum Number of the timer
 *  \param ch Channel
 *  \param value Value to compare against
 *  \param cmp_ev Event channel to strobe on compare. -1 is none.
 *  \return 0 for success, errors.h otherwise
 */
uint8_t timer_comp(timer_portname_t timer, timer_chan_t ch, uint16_t value,
	uint8_t cmp_ev);

/** \brief set the given channel overflow behaviour
 *
 *  The ovf_hook function must return void and accept no arguments.
 *  May be provided as NULL if not required.
 *
 *  Note: In PWM modes, overflow also resets the state of the
 *  associated pins automatically, if set as output
 *
 *  \param timernum Number of the timer
 *  \param ovf_ev Even channel to strobe on overflow, -1 is none.
 *  \return 0 for success, errors.h otherwise
 */
uint8_t timer_ovf(timer_portname_t timer, uint8_t ovf_ev);

/** \brief Set the given channel compare (value only)
 *
 *  This is a lightweight version of timer_comp() designed to just
 *  change the compare value for the channel.
 *
 *  \param timernum Number of the timer
 *  \param ch Channel
 *  \param value Compare value
 *  \return 0 for success, errors.h otherwise
 */
uint8_t timer_comp_val(timer_portname_t timer, timer_chan_t ch, uint16_t value);

/** \brief Force a specific count value
 *
 *  Note: timer must be stopped for this to be safe.
 *  \param timernum Number of the timer
 *  \param ch Channel of the timer
 *  \param value Count value
 *  \return 0 for success, errors.h otherwise
 */
uint8_t timer_count(timer_portname_t timer, uint16_t value);

/** \brief Turn off a given channel
 *  \param timernum Number of the timer
 *  \param ch Channel of the timer
 *  \return 0 for success, errors.h otherwise
 */
uint8_t timer_comp_off(timer_portname_t timer, timer_chan_t ch);

#ifdef __cplusplus
}
#endif

#endif // TIMER_H_INCLUDED
