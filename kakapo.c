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

 /** \file
  *  \brief Kakapo dev board support functions. */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "global.h"
#include "kakapo.h"
#include "clock.h"

void kakapo_init(void) {
    /* LEDs first */
    PORTE.DIRSET = (PIN2_bm | PIN3_bm);
    PORTE.OUTCLR = (PIN2_bm | PIN3_bm);

    /* we use DFLL, so fire up the external watch crystal */
    clock_xosc(xosc_32khz,xosc_lowspeed,0,0);
    clock_osc_run(osc_xosc);

    /* now system clock */
    switch (F_CPU) {
        case 2000000:
            /* just enable DFLL */
            clock_dfll_enable(osc_rc2mhz,dfll_xosc32khz);
            /* and switch to it */
            clock_sysclk(sclk_rc2mhz);
            break;
        case 32000000:
            /* enable the 32MHz OSC */
            clock_osc_run(osc_rc32mhz);
            /* enable DFLL for 32MHz OSC */
            clock_dfll_enable(osc_rc32mhz,dfll_xosc32khz);
            /* switch to it */
            clock_sysclk(sclk_rc32mhz);
            break;
    }

    /* enable interrupts */
    PMIC.CTRL = PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
 	sei();

 	return;
}
