/* Copyright (C) 2013-2014 David Zanetti
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

#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

/* Fallback in case it's not defined */
#ifndef F_CPU
#define F_CPU 32000000
#endif

/* Some releases of the avr-libc header files used an incorrect name for
 * the ADC internal voltage references. This will tell adc.c to use the
 * bad names. This should only be enabled if your get error messages
 * that "ADC_REFSEL_INTVCC_gc" is undefined.
 */
//#define BUG_AVRLIBC_ADC_REFNOINTPREFIX

/* the following table maps known XMEGA chips into families. Most only vary by size between
 * chips, and what perpherials exist are usually by family.
 *
 * Notable exceptions: 256A3BU and 256A3B have unusual pin usage on PORTE
 */

#if defined(__AVR_ATxmega64A1U__) || defined(__AVR_ATxmega128A1U__)

#define _xmega_type_A1U

#elif defined(_AVR_ATxmega128A3U__) || defined(__AVR_ATxmega192A3U__) || defined (__AVR_ATxmega256A3__)

#define _xmega_type_A3U

#elif defined (__AVR_ATxmega256A3BU__)

#define _xmega_type_256A3BU

#elif defined(__AVR_ATxmega16A4U__) || defined(__AVR_ATxmega32A4U__) || defined(__AVR_ATxmega64A4U__) || defined(__AVR_ATxmega128A4U__)

#define _xmega_type_A4U

#elif defined(__AVR_ATxmega64A1__) || defined (__AVR_ATxmega128A1__)

#define _xmega_type_A1

#elif defined (__AVR_ATxmega64A3__) || defined (__AVR_ATxmega128A3__) || defined (__AVR_ATxmega192A3__) || defined(__AVR_ATxmega256A3__)

#define _xmega_type_A3

#elif defined (__AVR_ATxmega256A3B__)

#define _xmega_type_256A3B

#elif defined(__AVR_ATxmega16A4__) || defined(__AVR_ATxmega32A4__)

#define _xmega_type_A4

#elif defined (__AVR_ATxmega64B1__) || defined (__AVR_ATxmega128B1__)

#define _xmega_type_B1

#elif defined (__AVR_ATxmega64B3__) || defined (__AVR_ATxmega128B3__)

#define _xmega_type_B3

#elif defined(__AVR_ATxmega32C3__) || defined (__AVR_ATxmega64C3__) || defined (__AVR_ATxmega128C3__) || defined(__AVR_ATxmega192C3__) || defined (__AVR_ATxmega256C3__) || defined (__AVR_ATxmega384C3__)

#define _xmega_type_C3

#elif defined(__AVR_ATxmega16C4__) || defined (__AVR_ATxmega32C4__)

#define _xmega_type_C4

#elif defined(__AVR_ATxmega32D3__) || defined (__AVR_ATxmega64D3__) || defined (__AVR_ATxmega128D3__) || defined (__AVR_ATxmega192D3__) || defined (__AVR_ATxmega256D3__) || defined (__AVR_ATxmega384D3__)

#define _xmega_type_D3

#elif defined(__AVR_ATxmega16D4__) || defined(__AVR_ATxmega32D4__) || defined(__AVR_ATxmega64D4__) || defined(__AVR_ATxmega128D4__)

#define _xmega_type_D4

#elif defined (__AVR_ATxmega8E5__) || defined(__AVR_ATxmega16E5__) || defined(__AVR_ATxmega32E5__)

#define _xmega_type E5

#endif // defined(__AVR

/* work out how many channels we have on this hardware */
#ifdef EVSYS_CH4MUX
#define MAX_EVENT 8
#else
#define MAX_EVENT 4
#endif // EVSYS_CH4MUX

#endif // GLOBAL_H_INCLUDED
