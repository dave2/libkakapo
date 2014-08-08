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
#include "errors.h"
#include "wdt.h"

/* normal mode configuration */
int wdt_normal(wdt_clk_t timeout) {
    uint8_t w;

    /* discard invalid timeout values */
    if (timeout > wdt_8kclk) {
        return -EINVAL;
    }

    /* check to see we're not running already */
    if (WDT.CTRL & WDT_ENABLE_bm) {
        return -ENOTREADY;
    }

    /* Apply config and run the watchdog timer */
    w = (timeout << WDT_PER_gp) | WDT_CEN_bm | WDT_ENABLE_bm;
    CCP = CCP_IOREG_gc;
    WDT.CTRL = w;

    /* all done! */
    return 0;
}

/* windowed mode configuration */
int wdt_window(wdt_clk_t closed, wdt_clk_t open) {
    uint8_t w;

    /* discard invalid timeout values */
    if (closed > wdt_8kclk || open > wdt_8kclk) {
        return -EINVAL;
    }

    /* if we're running the WDT already, nah */
    if (WDT.CTRL & WDT_ENABLE_bm) {
        return -ENOTREADY;
    }

    /* start with the closed setup */
    w = (closed << WDT_WPER_gp) | WDT_WCEN_bm | WDT_WEN_bm;
    CCP = CCP_IOREG_gc;
    WDT.WINCTRL = w;

    /* now the open section */
    w = (open << WDT_PER_gp) | WDT_CEN_bm | WDT_ENABLE_bm;
    CCP = CCP_IOREG_gc;
    WDT.CTRL = w;

    /* all done */
    return 0;
}
