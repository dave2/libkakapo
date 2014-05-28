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

#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

#define F_CPU 32000000

#if defined(__AVR_ATxmega16D4__) || defined(__AVR_ATxmega32D4__) || defined(__AVR_ATxmega64D4__) || defined(__AVR_ATxmega128D4__)

#define _xmega_type_D4

#elif defined(__AVR_ATxmega16A4U__) || defined(__AVR_ATxmega32A4U__) || defined(__AVR_ATxmega64A4U__) || defined(__AVR_ATxmega128A4U__)

#define _xmega_type_A4U

#endif // defined(__AVR

#endif // GLOBAL_H_INCLUDED
