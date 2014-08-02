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
#include "global.h"
#include "clock.h"
#include "errors.h"

/* run a specific oscilator */
int clock_osc_run(osc_type_t osc) {
    /* set the osc to run, then return when it's ready */
    switch (osc) {
        case osc_pll:
            OSC.CTRL |= OSC_PLLEN_bm;
            while (!(OSC.STATUS & OSC_PLLRDY_bm)); /* wait for stable */
            break;
        case osc_xosc:
            OSC.CTRL |= OSC_XOSCEN_bm;
            while (!(OSC.STATUS & OSC_XOSCRDY_bm)); /* wait for stable */
            break;
        case osc_rc32khz:
            OSC.CTRL |= OSC_RC32KEN_bm;
            while (!(OSC.STATUS & OSC_RC32KRDY_bm)); /* wait for stable */
            break;
        case osc_rc32mhz:
            OSC.CTRL |= OSC_RC32MEN_bm;
            while (!(OSC.STATUS & OSC_RC32MRDY_bm)); /* wait for stable */
            break;
        case osc_rc2mhz:
            OSC.CTRL |= OSC_RC2MEN_bm;
            while (!(OSC.STATUS & OSC_RC2MRDY_bm)); /* wait for stable */
            break;
        default:
            return -ENODEV;
            break;
    }
    return 0;
}

/* eanble DFLL for a specific oscilator */
int clock_dfll_enable(osc_type_t osc, dfll_src_t source) {

    switch (osc) {
        case osc_rc32mhz:
            /* need to do this per source */
            switch (source) {
                case dfll_rc32khz:
                    /* is the source running? */
                    if (!(OSC.STATUS & OSC_RC32KRDY_bm)) {
                        return -ENOTREADY;
                    }
                    /* set it as the source */
                    OSC.DFLLCTRL |= OSC_RC32MCREF_RC32K_gc;
                    break;
                case dfll_xosc32khz:
                    /* first check to see if it's configured sanely */
                    if ((OSC.XOSCCTRL & OSC_XOSCSEL_gm) != OSC_XOSCSEL_32KHz_gc) {
                        /* this is not running in 32kHz mode, abort */
                        return -EINVAL;
                    }
                    /* is the source running? */
                    if (!(OSC.STATUS & OSC_XOSCRDY_bm)) {
                        return -ENOTREADY;
                    }
                    OSC.DFLLCTRL |= OSC_RC32MCREF_XOSC32K_gc;
                    break;
                default:
                    return -EINVAL;
            }

        	/* enable DFLL for the 32MHz clock */
            DFLLRC32M.CTRL |= DFLL_ENABLE_bm;
            break;

        case osc_rc2mhz:

            switch (source) {
                case dfll_rc32khz:
                    /* is the source running? */
                    if (!(OSC.STATUS & OSC_RC32KRDY_bm)) {
                        return -ENOTREADY;
                    }
                    /* set it as the source */
                    OSC.DFLLCTRL |= OSC_RC2MCREF_RC32K_gc;
                    break;
                case dfll_xosc32khz:
                    /* first check to see if it's configured sanely */
                    if ((OSC.XOSCCTRL & OSC_XOSCSEL_gm) != OSC_XOSCSEL_32KHz_gc) {
                        /* this is not running in 32kHz mode, abort */
                        return -EINVAL;
                    }
                    /* is the source running? */
                    if (!(OSC.STATUS & OSC_XOSCRDY_bm)) {
                        return -ENOTREADY;
                    }
                    OSC.DFLLCTRL |= OSC_RC2MCREF_XOSC32K_gc;
                    break;
                default:
                    return -EINVAL;
            }

        	/* enable DFLL for the 2MHz clock */
            DFLLRC2M.CTRL |= DFLL_ENABLE_bm;
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

int clock_divisor(sclk_psa_t diva, sclk_psbc_t divbc) {
    uint8_t ps;

    /* check the params first */
    if (diva > sclk_psa_512 || divbc > sclk_psbc_22) {
        return -EINVAL;
    }
    ps = (diva << CLK_PSADIV_gp) | divbc;
    /* register protection */
    CCP = CCP_IOREG_gc;
    CLK.PSCTRL = ps; /* must happen within 4 clocks */

    return 0;
}

int clock_xosc(xosc_type_t type, xosc_freqrange_t freqrange, uint8_t drive, uint8_t lpm32khz) {
    /* check for invalid ranges */
    if (freqrange > xosc_lowspeed) {
        return -EINVAL;
    }
    /* apply configuration */
    switch (type) {
        case xosc_extclk:
            OSC.XOSCCTRL = OSC_XOSCSEL_EXTCLK_gc;
            break;
        case xosc_32khz:
            OSC.XOSCCTRL = OSC_XOSCSEL_32KHz_gc;
            break;
        case xosc_xtal_256clk:
            OSC.XOSCCTRL = OSC_XOSCSEL_XTAL_256CLK_gc;
            break;
        case xosc_xtal_1kclk:
            OSC.XOSCCTRL = OSC_XOSCSEL_XTAL_1KCLK_gc;
            break;
        case xosc_xtal_16kclk:
            OSC.XOSCCTRL = OSC_XOSCSEL_XTAL_16KCLK_gc;
            break;
        default:
            return -EINVAL;
    }

    if (drive) {
        OSC.XOSCCTRL |= OSC_XOSCPWR_bm;
    }
    if (lpm32khz) {
        OSC.XOSCCTRL |= OSC_X32KLPM_bm;
    }
    if (freqrange != xosc_lowspeed) {
        OSC.XOSCCTRL |= ((freqrange << OSC_FRQRANGE_gp) & OSC_FRQRANGE_gm);
    }
    /* all done */
    return 0;
}

int clock_pll(pll_src_t source, uint8_t div2, uint8_t multiplier) {
    if (source > pll_xosc) {
        return -EINVAL;
    }
    if (multiplier > 31) {
        return -EINVAL;
    }
    /* apply configuration */
    OSC.PLLCTRL = multiplier;
    if (div2) {
        OSC.PLLCTRL |= OSC_PLLDIV_bm;
    }
    OSC.PLLCTRL |= (source << OSC_PLLSRC_gp);
    return 0;
}

int clock_sysclk(sclk_src_t source) {
    switch (source) {
        case sclk_pll:
            if (!(OSC.STATUS & OSC_PLLRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        case sclk_xosc:
            if (!(OSC.STATUS & OSC_XOSCRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        case sclk_rc32khz:
            if (!(OSC.STATUS & OSC_RC32KRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        case sclk_rc32mhz:
            if (!(OSC.STATUS & OSC_RC32MRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        case sclk_rc2mhz:
            if (!(OSC.STATUS & OSC_RC2MRDY_bm)) {
                return -ENOTREADY;
            }; /* wait for stable */
            break;
        default:
            return -EINVAL;
            break;
    }

    /* register is protected */
    CCP = CCP_IOREG_gc;
    CLK.CTRL = source; /* must happen within 4 clocks */
    /* note: there will be a large stall here when enabling this */

    return 0;
}

int clock_rtc(rtc_clk_src_t source) {

    switch (source) {
        case rtc_clk_ulp:
            /* ulp is enabled by default, so nothing to do here  */
            break;
        case rtc_clk_tosc:
        case rtc_clk_tosc32:
            /* tosc should be configured and running for 32kHz, in both of theses sources */
            if ((OSC.XOSCCTRL & OSC_XOSCSEL_gm) != OSC_XOSCSEL_32KHz_gc) {
                /* this is not running in 32kHz mode, abort */
                return -EINVAL;
            }
            /* check it's running */
            if (!(OSC.STATUS & OSC_XOSCRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        case rtc_clk_rcosc:
        case rtc_clk_rcosc32:
            /* check to see clock is running, for either mode */
            if (!(OSC.STATUS & OSC_RC32KRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        case rtc_clk_extclk:
            /* we *assume* this is a low frequency, but we have no way to check */
            if ((OSC.XOSCCTRL & OSC_XOSCSEL_gm) != OSC_XOSCSEL_EXTCLK_gc) {
                return -EINVAL;
            }
            /* check it's running */
            if (!(OSC.STATUS & OSC_XOSCRDY_bm)) {
                return -ENOTREADY;
            }
            break;
        default:
            return -EINVAL;
    }

    /* now apply the configuration which seems to be valid */
    CLK.RTCCTRL = (source << CLK_RTCSRC_gp) | CLK_RTCEN_bm;

    return 0;
}

