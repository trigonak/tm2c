/*
 *   File: pgas_dsl.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: PGAS memory for the service processes
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
#include "pgas_dsl.h"

static volatile void* pgas_dsl_mem;
#define PTR_ADD(ptr, plus) ((char*) (ptr) + (plus))

void 
pgas_dsl_init()
{
  pgas_dsl_mem = calloc(PGAS_DSL_SIZE_NODE, 1);
  assert(pgas_dsl_mem != NULL);
  PRINTD(" <><> allocated %ld B = %ld KiB = %ld MiB for pgas mem", 
	PGAS_DSL_SIZE_NODE, PGAS_DSL_SIZE_NODE / 1024, PGAS_DSL_SIZE_NODE / (1024 * 1024));
}

void
pgas_dsl_term()
{
  free((void*) pgas_dsl_mem);
}

inline int64_t
pgas_dsl_read(uint64_t offset)
{
  return *(int64_t*) PTR_ADD(pgas_dsl_mem, offset);
}

inline int64_t*
pgas_dsl_readp(uint64_t offset)
{
  return (int64_t*) PTR_ADD(pgas_dsl_mem, offset);
}


inline int32_t
pgas_dsl_read32(uint64_t offset)
{
  return *(int32_t*) PTR_ADD(pgas_dsl_mem, offset);
}


inline void
pgas_dsl_write(uint64_t offset, int64_t val)
{
  *(int64_t*) PTR_ADD(pgas_dsl_mem, offset) = val;
}

inline void
pgas_dsl_write32(uint64_t offset, int32_t val)
{
  *(uint32_t*) PTR_ADD(pgas_dsl_mem, offset) = val;
}
