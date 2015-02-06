#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

/* Copyright (C) 2015 David Zanetti
 *
 * This file is part of libkakapo
 *
 * libkakapo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * libkakapo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libkapapo.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

/* This is a debugging macro, which allows you to enable debugs */
#include <stdarg.h>

/* disable debug by default */
#ifndef KAKAPO_DEBUG_LEVEL
#define KAKAPO_DEBUG_LEVEL 0
#endif

/* if we haven't been given a file handle, disable debugging */
#ifndef KAKAPO_DEBUG_CHANNEL
#define KAKAPO_DEBUG_LEVEL 0
#endif

#if (KAKAPO_DEBUG_LEVEL > 0)
#define k_err_P(M,...)   { fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("[err] %s:%d "),__FILE__,__LINE__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL, M, ##__VA_ARGS__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("\r\n")); }
#else
#define k_err_P(M,...)
#endif

#if (KAKAPO_DEBUG_LEVEL > 1)
#define k_warn_P(M,...)  { fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("[warn] %s:%d "),__FILE__,__LINE__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL, M, ##__VA_ARGS__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("\r\n")); }
#else
#define k_warn_P(M,...)
#endif

#if (KAKAPO_DEBUG_LEVEL > 2)
#define k_info_P(M,...)  { fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("[info] %s:%d "),__FILE__,__LINE__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL, M, ##__VA_ARGS__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("\r\n")); }
#else
#define k_info_P(M,...)
#endif

#if (KAKAPO_DEBUG_LEVEL > 3)
#define k_debug_P(M,...) { fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("[debug] %s:%d "),__FILE__,__LINE__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL, M, ##__VA_ARGS__); \
                             fprintf_P(KAKAPO_DEBUG_CHANNEL,PSTR("\r\n")); }
#else
#define k_debug_P(M,...)
#endif

#endif // DEBUG_H_INCLUDED
