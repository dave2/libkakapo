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

#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include <errno.h>

/** \file
 *  \brief Error numbers we use in errno on top of the avr-libc
 *  defined ones
 */

#define ENONE 0 /**< No error */
#define EIO 5 /**< I/O error */
#define ENOMEM 12 /**< Out of memory */
#define EBUSY 16 /**< Device or resource is busy */
#define ENODEV 19 /**< No such device */
#define EINVAL 22 /**< Invalid Argument */
#define EBAUD 134 /**< Baud rate can't be achieved */
#define ENOTREADY 135 /**< Hardware is not ready */

#endif // ERRORS_H_INCLUDED
