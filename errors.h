#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

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

#include <errno.h>

/** \file
 *  \brief Error numbers we use in errno on top of the avr-libc
 *  defined ones
 */

#define ENONE 0 /**< No error */
#define EIO 5 /**< I/O error */
#define ENOMEM 12 /**< Out of memory */
#define ENODEV 19 /**< No such device */
#define EINVAL 22 /**< Invalid Argument */
#define EBAUD 134 /**< Baud rate can't be achieved */

#endif // ERRORS_H_INCLUDED
