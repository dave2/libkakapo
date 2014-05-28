#ifndef CLOCK_H_INCLUDED
#define CLOCK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

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

/** \file
 *  \brief XMEGA clock setup, incl. RTC
 *
 *  Set up clocking system with F_CPU defined clock speed. Optionally
 *  allows per4 and per2 clocks to be 4x and 2x as required. In all
 *  cases, DFLL is used to improve RC oscilator stability.
 *
 *  Also establishes RTC clock and functions to support reading it
 *
 *  Note: Only the following F_CPU values are supported:
 *  - 1MHz (2MHz RC OSC / 2)
 *  - 2MHz (2MHz RC OSC)
 *  - 4MHz (32MHz RC OSC / 8)
 *  - 8MHz (32MHz RC OSC / 4)
 *  - 12MHz (2MHz RC OSC, 6x PLL)
 *  - 16MHz (32MHz RC OSC / 2)
 *  - 32MHz (32MHZ RC OSC)
 *
 *  With per4/per2 enabled, the same F_CPU values are supported with
 *  the following methods:
 *  - 1MHz CPU, 2/4MHz per2/4 (32MHz RC OSC, /8, /2, /2)
 *  - 2MHz CPU, 4/8MHz per2/4 (32MHz RC OSC, /4, /2, /2)
 *  - 4MHz CPU, 8/16MHz per2/4 (32MHz RC OSC, /2, /2, /2)
 *  - 8MHz CPU, 16/32MHz per2/4, (32MHz RC OSC, /1, /2, /2)
 *  - 12MHz CPU, 24/48MHz per2/4, (2MHz RC OSC, 24x PLL, /1, /2, /2)
 *  - 16MHz CPU, 32/64MHz per2/4, (32MHz RC OSC, 2x PLL, /1, /2, /2)
 *  - 32MHz CPU, 64/128MHz per2/4, (32Mhz RC OSC, 4x PLL, /1, /2, /2)
 *
 */

/*
 *  FIXME: support 12MHz for <2.7V operation, using 2MHz RC, 6xPLL
 */

/** \brief Configure system clock and RTC
 *
 *  Clock system will be configured to use F_CPU as the CPU frequency.
 *  All perpherial clocks run at same frequency as CPU. See
 *  sysclk_init_perhigh() as alternative if true per4 and per2 clocks
 *  are needed.
 */

void sysclk_init(void);

/** \brief Configure system clock, with 4x and 2x perperhial clock
 *  support. Also configures RTC.
 *
 *  This is the same as sysclk_init(), runs CPU clock at F_CPU but
 *  runs per4 and per2 clocks at 4x and 2x CPU clock. This consumes
 *  more power (as it needs the PLL) for 16MHz and 32MHz F_CPU.
 */
void sysclk_init_perhigh(void);

/** \brief Return the current uptime
 *
 *  This function writes the current uptime in seconds and fractions
 *  of a second to the given variables. Note: fractions are expressed
 *  as a fixed point 16-bit number, ie 0.5 seconds == 32768.
 *  Note: either field can be NULL to ignore this portion.
 *
 *  \param *seconds pointer to seconds portion, must be uint32_t
 *  \param *faction pointer to fraction portion, must be uint16_t
 */
void sysclk_uptime(uint32_t *seconds, uint16_t *fraction);

/** \brief Return current fractions of a second
 *
 *  This function returns the current fraction of a second over
 *  a range 0-1023, roughly equivilent to milliseconds. The
 *  difference between this value and wall-clock milliseconds
 *  may be up to 2%, however it is very fast to extract from
 *  the hardware.
 *
 *  Note: unlike sysclk_uptime(), this is not a fixed point fraction.
 *
 *  \return current fraction of seconds (10-bit)
 */
uint16_t sysclk_ticks(void);

#ifdef __cplusplus
}
#endif

#endif // CLOCK_H_INCLUDED
