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

#ifndef WDT_H_INCLUDED
#define WDT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* while this does dupe bits of avr-libc, this version uses clock
 * cycle counts, as per datasheets, and not approximate wall clock
 * time. The counts here are what the datasheet states. */

typedef enum {
    wdt_8clk = 0, /**< 8 ULP ticks, ~8ms */
    wdt_16clk, /**< 16 ULP ticks, ~16ms */
    wdt_32clk, /**< 32 ULP ticks, ~32ms */
    wdt_64clk, /**< 64 ULP ticks, ~64ms */
    wdt_128clk, /**< 128 ULP ticks, ~128ms */
    wdt_256clk, /**< 256 ULP ticks, ~256ms */
    wdt_512clk, /**< 512 ULP ticks, ~512ms */
    wdt_1kclk, /**< 1000 ULP ticks, ~1.0s */
    wdt_2kclk, /**< 2000 ULP ticks, ~2.0s */
    wdt_4kclk, /**< 4000 ULP ticks, ~4.0s */
    wdt_8kclk, /**< 8000 ULP ticks, ~8.0s */
} wdt_clk_t;

/** \brief Start watchdog in normal mode
 *
 *  In normal mode the watchdog can be reset at any point
 *  before the timeout expires. If the timeout is reached
 *  without the watchdog being reset, the MCU will hard
 *  reset.
 *
 *  \param timeout How many 1kHz ULP clocks to timeout on
 *  \return 0 on success, errors.h otherwise
 */
int wdt_normal(wdt_clk_t timeout);

/** \brief Start watchdog in window mode
 *
 *  In windowed mode, the watchdog must be reset only
 *  in the "open" section, reset before the window opens
 *  or reset after the end of the open section will hard
 *  reset the MCU.
 *
 *  \param closed How many 1kHz ULP clocks the WDT will
 *  consider is too early to be asked to reset
 *  \param open How many 1kHz ULP clocks after the closed
 *  period will a reset be allowed
 *  \return 0 on success, errors.h otherwise
 */
int wdt_window(wdt_clk_t closed, wdt_clk_t open);

/** \brief A macro to execute the watchdog reset instruction
 */
/* This may already be defined by avr-libc, if someone included it's wdt.h */
#ifndef wdt_reset
#define wdt_reset() __asm__ __volatile__("wdr")
#endif

#ifdef __cplusplus
}
#endif

#endif // WDT_H_INCLUDED
