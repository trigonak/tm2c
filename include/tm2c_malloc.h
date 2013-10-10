/*
 *   File: tm2c_malloc.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: a very very simple object allocator
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

#ifndef TM2C_LIB_H
#define TM2C_LIB_H

#include <stdint.h>
#include "tm2c_types.h"

#define TM2C_SHMEM_SIZE (TM2C_SHMEM_SIZE_MB * 1024 * 1024LL)

void tm2c_shmalloc_set(void* mem);
void tm2c_shmalloc_init(size_t size);
void tm2c_shmalloc_term();
void* tm2c_shmalloc(size_t size);
void tm2c_shfree(void* ptr);

#endif
