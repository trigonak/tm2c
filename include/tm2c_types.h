/*
 *   File: tm2c_types.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: types used by TM2C
 *   This file is part of TM2C
 *
 *   Copyright (C) 2013  Vasileios Trigonakis
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef TM2C_TYPES_H
#define TM2C_TYPES_H

#include <inttypes.h>
#include <stdint.h>

#if SIZE_MAX == 4294967295
#  define PRIxSIZE PRIx32
#elif SIZE_MAX > 4294967295
#  define PRIxSIZE PRIx64
#else
#  define PRIxSIZE x
#endif

typedef enum
  {
    FALSE, //0
    TRUE //1
  } BOOLEAN;

typedef uint32_t nodeid_t;
typedef void* tm_addr_t;

/*
 * The format in printf to represent tm_addr_t
 */
#define PRIxA "p"

#ifdef PGAS
/*
 * With PGAS, addresses that are handled by the dslock nodes are offsets
 */
typedef size_t tm_intern_addr_t;

/*
 * The format in printf to represent this address
 */
#  define PRIxIA "#0" PRIxSIZE 
#else
/*
 * However, for the non-PGAS code, they are uintptr_t 
 */
typedef uintptr_t tm_intern_addr_t;
#  define PRIxIA PRIxPTR
#endif

#endif
