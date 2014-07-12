#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

/* Copyright (C) 2014 David Zanetti
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

/* RTC module public interface */

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Prescaler for RTC clock */
typedef enum {
	rtc_off = 0, /**< Stop running the RTC */
	rtc_div1, /**< CLKrtc/1 */
	rtc_div2, /**< CLKrtc/2 */
	rtc_div4, /**< CLKrtc/4 */
	rtc_div8, /**< CLKrtc/8 */
	rtc_div64, /**< CLKrtc/64 */
	rtc_div256, /**< CLKrtc/256 */
	rtc_div1024, /**< CLKrtc/1024 */
} rtc_clk_div_t;

/** \brief Initalise the RTC.
 *
 *  The cmp_hook function must return void, and accept no args. The RTC
 *  module only has one compare register. Can be NULL if unused.
 *
 *  The ovf_hook function is similar, return void and accept no args.
 *  It also may be NULL if not required.
 *
 *  \param period Timer period (ie, TOP value)
 *  \param cmp_hook Function to invoke on compare
 *  \param ovf_hook Function to invoke on overflow
 *  \param cmp_ev Event channel to strobe on compare, -1 is none
 *  \param ovf_ev Event channel to strobe on overflow, -1 is none
 *  \return 0 on success, errors.h otherwise
 */
int rtc_init(uint16_t period, void (*cmp_hook)(), void (*ovf_hook)());

/** \brief Clock the RTC at the given divisor.
 *
 *  This sets the RTC divider from its clock source. rtc_off stops the
 *  RTC.
 *
 *  Note: Before calling this function, you should ensure that the clock
 *  source has been configured where appropriate, using clock_xosc() and
 *  clock_osc_run(), and then selected with clock_rtc().
 *
 *  \param div Divisor to use.
 *  \returns 0 on success, errors.h otherwise
 */
int rtc_div(rtc_clk_div_t div);

/** \brief Set the RTC compare.
 *
 *  The RTC has one RTC compare register. If cmp_hook() was defined on
 *  init, then it is called when the compare matches. An event channel
 *  can also be passed to strobe on compare.
 *
 *  If this value is set higher than the period, then no compare will
 *  ever fire (either as the hook or on the event channel).
 *
 *  \param value Compare value
 *  \return 0 on success, errors.h otherwise.
 */
int rtc_comp(uint16_t value);

/** \brief Set the RTC to a specific value
 *
 *  Note: RTC must be stopped for this function.
 *
 *  \param value Value to be set
 *  \return 0 on success, errors.h otherwise
 */
int rtc_setcount(uint16_t value);

/** \brief Retrieve the current RTC value
 *
 *  This is needed to ensure a clean sync between RTC domain
 *  and the main clock domain.
 *
 *  \return the current RTC count value
 */
uint16_t rtc_count(void);

#ifdef __cplusplus
}
#endif

#endif // RTC_H_INCLUDED
