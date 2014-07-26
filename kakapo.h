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

#ifndef KAKAPO_H_INCLUDED
#define KAKAPO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  \brief Various functions related to the Kakapo development board.
 *
 *  This contains various useful setup functions and other interfaces
 *  specific to the Kakapo dev board.
 *
 *  These functions are not required for other boards, but you may wish
 *  to use a similar initalisation method.
 */

/** \brief Initalise Kakapo hardware
 *
 *  Applies the following configuration:
 *  - System clock set up to match F_CPU if set to 2MHz or 32MHz.
 *  - Internal RC oscilators use DFLL off watch crystal for stablity.
 *  - Pins PE2 and PE3 (which have LEDs) are set as outputs.
 *  - Enables all interrupt levels and global interrupt flag.
 */
void kakapo_init(void);

#ifdef __cplusplus
}
#endif

#endif // KAKAPO_H_INCLUDED
