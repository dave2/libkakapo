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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "global.h"
#include "clock.h"

uint32_t uptime;

void sysclk_init(void) {

#ifndef F_CPU
#error F_CPU must be defined for clock functions
#endif

	/* fire up 32.768kHz external crystal */
	OSC.XOSCCTRL |= OSC_XOSCSEL_32KHz_gc;
	OSC.CTRL |= OSC_XOSCEN_bm;
	while (!(OSC.STATUS & OSC_XOSCRDY_bm));

#if F_CPU == 1000000 || F_CPU == 2000000
	/* Since 2MHz RC is default, just enable DFLL and divide as appropriate */

	/* enable DFLL for the 2MHz clock */
	OSC.DFLLCTRL |= OSC_RC2MCREF_XOSC32K_gc;
	DFLLRC2M.CTRL |= DFLL_ENABLE_bm;

#if F_CPU == 1000000
	/* for 1MHz, we need to divide the clock down */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_2_gc | CLK_PSBCDIV_1_1_gc);
#endif

#elif F_CPU == 4000000 || F_CPU == 8000000 || F_CPU == 16000000 || F_CPU == 32000000
	/* for this clock case, divide down the 32MHz RC OSC */

	/* enable DFLL for 32MHz clock */
	OSC.DFLLCTRL |= OSC_RC32MCREF_XOSC32K_gc;
	DFLLRC32M.CTRL |= DFLL_ENABLE_bm;

	OSC.CTRL |= OSC_RC32MEN_bm; /* run the 32MHz OSC */
    while (!(OSC.STATUS & OSC_RC32MRDY_bm)); /* wait for it to stablise */

    /* now divide down the output from the clock matching the desired freq */
#if F_CPU == 4000000
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_8_gc | CLK_PSBCDIV_1_1_gc);
#elif F_CPU == 8000000
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_4_gc | CLK_PSBCDIV_1_1_gc);
#elif F_CPU == 16000000
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_2_gc | CLK_PSBCDIV_1_1_gc);
#elif F_CPU == 32000000
	/* technically not needed, but here for completeness */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_1_gc | CLK_PSBCDIV_1_1_gc);
#endif
	/* and attach the system clock to it */
	CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
#elif F_CPU == 12000000
	/* 12MHz clock is obtained using PLL 6x of 2MHz RC OSC */
	/* enable DFLL for the 2MHz clock */
	OSC.DFLLCTRL |= OSC_RC2MCREF_XOSC32K_gc;
	DFLLRC2M.CTRL |= DFLL_ENABLE_bm;
	/* now fed the 2MHz clock to the PLL, make for 6x */
    OSC.PLLCTRL = 6; /* 1x input -> 12MHz */
    OSC.PLLCTRL |= OSC_PLLSRC_RC2M_gc;

	/* start the PLL */
    OSC.CTRL |= OSC_PLLEN_bm;
    while (!(OSC.STATUS & OSC_PLLRDY_bm)); /* wait for stable */

    /* okay, we're now good to change to the PLL output */
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_PLL_gc;
#else
#error Invalid F_CPU specified for clock functions
#endif
	/* reset uptime */
	uptime = 0;

	/* select RTC source to be 32.768kHz external crystal */
	CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;

	/* configure RTC */
	RTC.CNT = 0;
	RTC.PER = 32767;
	RTC.INTCTRL = RTC_OVFINTLVL_LO_gc; /* these are very low prio */
	/* FIXME: when is this needed? */
	//while (RTC.STATUS & RTC_SYNCBUSY_bm);
	/* null body */

	/* run it */
	RTC.CTRL = RTC_PRESCALER_DIV1_gc;

    return;
}

void sysclk_init_perhigh(void) {

#ifndef F_CPU
#error F_CPU must be defined for clock functions
#endif

	/* fire up 32.768kHz external crystal */
	OSC.XOSCCTRL |= OSC_XOSCSEL_32KHz_gc;
	OSC.CTRL |= OSC_XOSCEN_bm;
	while (!(OSC.STATUS & OSC_XOSCRDY_bm));

#if F_CPU == 1000000 || F_CPU == 2000000 || F_CPU == 4000000 || F_CPU == 8000000
	/* enable DFLL for 32MHz clock */
	OSC.DFLLCTRL |= OSC_RC32MCREF_XOSC32K_gc;
	DFLLRC32M.CTRL |= DFLL_ENABLE_bm;

	OSC.CTRL |= OSC_RC32MEN_bm; /* run the 32MHz OSC */
    while (!(OSC.STATUS & OSC_RC32MRDY_bm)); /* wait for it to stablise */

#if F_CPU == 1000000
	/* 4, 2, 1MHz clocks */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_8_gc | CLK_PSBCDIV_2_2_gc);
#elif F_CPU == 2000000
	/* 8, 4, 2MHz clocks */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_4_gc | CLK_PSBCDIV_2_2_gc);
#elif F_CPU == 4000000
	/* 16, 8, 4MHz clocks */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_2_gc | CLK_PSBCDIV_2_2_gc);
#elif F_CPU == 8000000
	/* 32, 16, 8MHz clocks */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_1_gc | CLK_PSBCDIV_2_2_gc);
#endif
	/* and attach the system clock to it */
	CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
#elif F_CPU == 16000000 || F_CPU == 32000000
	/* enable DFLL for 32MHz clock */
	OSC.DFLLCTRL |= OSC_RC32MCREF_XOSC32K_gc;
	DFLLRC32M.CTRL |= DFLL_ENABLE_bm;

	OSC.CTRL |= OSC_RC32MEN_bm; /* run the 32MHz OSC */
    while (!(OSC.STATUS & OSC_RC32MRDY_bm)); /* wait for it to stablise */

	/* PLL needs to be run for these cases */
#if F_CPU == 16000000
    OSC.PLLCTRL = 2; /* 2x input -> 64MHz */
    OSC.PLLCTRL |= OSC_PLLSRC_RC32M_gc;
    /* 64, 32, 16MHz clocks */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_1_gc | CLK_PSBCDIV_2_2_gc);
#elif F_CPU == 3200000
    OSC.PLLCTRL = 4; /* 4x input -> 128MHz */
    OSC.PLLCTRL |= OSC_PLLSRC_RC32M_gc;
    /* 128, 64, 32MHz clocks */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_1_gc | CLK_PSBCDIV_2_2_gc);
#endif
	/* start the PLL */
    OSC.CTRL |= OSC_PLLEN_bm;
    while (!(OSC.STATUS & OSC_PLLRDY_bm)); /* wait for stable */
	/* and attach the system clock to it */
	CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_PLL_gc;
#elif F_CPU == 12000000
	/* 12MHz clock is obtained using PLL 24x of 2MHz RC OSC, then divide */
	/* enable DFLL for the 2MHz clock */
	OSC.DFLLCTRL |= OSC_RC2MCREF_XOSC32K_gc;
	DFLLRC2M.CTRL |= DFLL_ENABLE_bm;

	/* divide down */
	CCP = CCP_IOREG_gc;
    CLK.PSCTRL = (CLK_PSADIV_1_gc | CLK_PSBCDIV_2_2_gc);

	/* now fed the 2MHz clock to the PLL, make for 6x */
    OSC.PLLCTRL = 24; /* 24x input -> 48MHz */
    OSC.PLLCTRL |= OSC_PLLSRC_RC2M_gc;

	/* start the PLL */
    OSC.CTRL |= OSC_PLLEN_bm;
    while (!(OSC.STATUS & OSC_PLLRDY_bm)); /* wait for stable */

    /* okay, we're now good to change to the PLL output */
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_PLL_gc;
#else
#error Invalid F_CPU specified for clock functions
#endif
    return;
}

void sysclk_uptime(uint32_t *seconds, uint16_t *fraction) {
	/* FIXME: glitchy? */
	if (seconds) {
		*seconds = uptime;
	}
	if (fraction) {
		*fraction = (RTC.CNT << 1); /* left shift by 1 bit to be fixed point */
	}
}

uint16_t sysclk_millis(void) {
	return (RTC.CNT >> 6); /* divide by 32 = 1024Hz */
}

ISR(RTC_OVF_vect) {
	uptime++;
}
