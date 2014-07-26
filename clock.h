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

#ifndef CLOCK_H_INCLUDED
#define CLOCK_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  \brief XMEGA clock interface.
 *
 *  This provides a number of wrapper functions around the system, rtc,
 *  and oscilator configuration. The clocking system is run-time
 *  configurable rather than set by fuses.
 */

/** \brief What clock source is RTC using */
typedef enum {
    rtc_clk_ulp = 0, /**< RTC clocked from internal ULP 32kHz, divided to 1kHz */
    rtc_clk_tosc, /**< RTC clocked from 32.768kHz on TOSC, divded to 1.024kHz */
    rtc_clk_rcosc, /**< RTC clocked from 32.768kHz internal, divided to 1.024kHz */
    rtc_clk_reserved1, /**< Reserved */
    rtc_clk_reserved2, /**< Reserved */
    rtc_clk_tosc32, /**< RTC clocked from 32.768kHz on TOSC */
    rtc_clk_rcosc32, /**< RTC clocked from 32.768kHz internal */
    rtc_clk_extclk, /**< RTC clocked from external clock on TOSC */
} rtc_clk_src_t;

/** \brief System clock source options */
typedef enum {
    sclk_rc2mhz = 0, /**< System clocked from internal 2MHz OSC */
    sclk_rc32mhz, /**< System clocked from internal 32MHz OSC */
    sclk_rc32khz, /**< System clocked from internal 32.768kHz OSC */
    sclk_xosc, /**< System clocked from external source (clock or osc) */
    sclk_pll, /**< System clocked from output of PLL */
} sclk_src_t;

/** \brief Prescaler A options */
typedef enum {
    sclk_psa_1 = 0, /**< Prescaler A No division */
    sclk_psa_2, /**< Prescaler A div 2 */
    sclk_psa_4, /**< Prescaler A div 4 */
    sclk_psa_8, /**< Prescaler A div 8 */
    sclk_psa_16, /**< Prescaler A div 16 */
    sclk_psa_32, /**< Prescaler A div 32 */
    sclk_psa_64, /**< Prescaler A div 64 */
    sclk_psa_128, /**< Prescaler A div 128 */
    sclk_psa_256, /**< Prescaler A div 256 */
    sclk_psa_512, /**< Prescaler A div 512 */
} sclk_psa_t;

/** \brief Prescaler B and C options */
typedef enum {
    sclk_psbc_11 = 0, /**< Prescaler B No division, Prescaler C No divison */
    sclk_psbc_12, /**< Prescaler B No division, Prescaler C div 2 */
    sclk_psbc_41, /**< Prescaler B div 4, Prescaler C No division */
    sclk_psbc_22, /**< Prescaler B div 2, Prescaler C div 2 */
} sclk_psbc_t;

/** \brief External high-speed crystal frequency range */
typedef enum {
    xosc_04to2mhz = 0, /**< 0.4MHz to 2MHz */
    xosc_2to9mhz, /**< 2MHz to 9MHz */
    xosc_9to12mhz, /**< 9MHz to 12MHz */
    xosc_12to16mhz, /**< 12MHz to 16MHz */
    xosc_lowspeed, /**< Low speed crystal mode */
} xosc_freqrange_t;

/** \brief External clock type selection */
typedef enum {
    xosc_extclk = 0, /**< External Clock on XOSC */
    xosc_32khz, /**< 32.768kHz watch crystal on TOSC */
    xosc_xtal_256clk, /**< External high-speed crystal, 256 clocks startup */
    xosc_xtal_1kclk, /**< External high-speed crystal, 1k clocks startup */
    xosc_xtal_16kclk, /**< External high-speed cyrstal, 16k clocks startup */
} xosc_type_t;

/** \brief PLL source selection */
typedef enum {
    pll_rc2mhz = 0, /**< PLL uses 2MHz internal OSC */
    pll_reserved, /**< Unused */
    pll_rc32m, /**< PLL uses 32MHz internal OSC */
    pll_xosc, /**< PLL uses external OSC */
} pll_src_t;

/** \brief DFLL reference selection */
typedef enum {
    dfll_rc32khz = 0, /**< DFLL uses internal 32.768kHz OSC */
    dfll_xosc32khz, /**< DFLL uses external 32.768kHz OSC */
} dfll_src_t;

/** \brief Oscilator selection */
typedef enum {
    osc_pll = 0, /**< Enable PLL */
    osc_xosc, /**< Enable External Oscilator */
    osc_rc32khz, /**< Enable Internal 32.768kHz OSC */
    osc_rc32mhz, /**< Enable Internal 32MHz OSC */
    osc_rc2mhz, /**< Enable Internal 2MHz OSC */
} osc_type_t;

/** \brief Enable a specific oscilator
 *
 *  Note: this must be called AFTER you have configured the oscilator.
 *
 *  \param osc Which oscilator to run
 *  \return 0 on success, errors.h otherwise
 */
int clock_osc_run(osc_type_t osc);

/** \brief Set up DFLL for the given OSC.
 *
 *  Note: not all OSC have support for DFLL, only the 32MHz internal RC
 *  and the 2MHz internal RC have DFLL support.
 *
 *  \param osc Oscilator to enable DFLL support for
 *  \param source Source to use for DFLL
 *  \return 0 on success, errors.h otherwise */
int clock_dfll_enable(osc_type_t osc, dfll_src_t source);

/** \brief Set the clock divisors
 *
 *  Clock divisors are in three stages: A for sclk source to be
 *  divided which is fed into per4 clock, B for output of A into
 *  per2 clock, and C for output of B into the system clock and
 *  per1 clock. C must not exceed 32MHz.
 *
 *  \param diva Divisor for stage A
 *  \param divbc Divisor for stage B and C
 *  \return 0 on success, errors.h otherwise
 */
int clock_divisor(sclk_psa_t diva, sclk_psbc_t divbc);

/** \brief External clock source and capabilities
 *
 *  \param type Type of external clock present
 *  \param freqrange The freqency range for the cyrstal
 *  \param drive Drive strength, 1 to increase, 0 for default
 *  \param lpm32khz Low power mode for 32.768kHz crystals, 1 for low power mode
 *  \return 0 on success, errors.h otherwise
 */
int clock_xosc(xosc_type_t type, xosc_freqrange_t freqrange, uint8_t drive, uint8_t lpm32khz);

/** \brief PLL setup
 *
 *  PLL can accept several osc sources and multiply them up
 *  to a higher frequency. This can then be selected as the
 *  main system clock source.
 *
 *  \param source PLL input source
 *  \param div2 Divide output by 2
 *  \param multiplier 1-31 clock multiplication
 *  \return 0 on success, errors.h otherwise
 */
int clock_pll(pll_src_t source, uint8_t div2, uint8_t multiplier);

/** \brief Set the system clock sources
 *
 *  This should generally be the last call in the process of config
 *  of the clocks.
 *
 *  \param source Source to use for system clock (fed into prescalers)
 *  \return 0 on success, errors.h otherwise
 */
int clock_sysclk(sclk_src_t source);

/** \brief Set the RTC clock source
 *
 *  \param source Source to use for RTC clock
 *  \return 0 on success, error.h otherwise
 */
int clock_rtc(rtc_clk_src_t source);

#ifdef __cplusplus
}
#endif

#endif // CLOCK_H_INCLUDED
