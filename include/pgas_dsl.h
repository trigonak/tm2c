/*
 *   File: pgas_dsl.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: interface for PGAS for service processes
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

#ifndef PGAS_DSL_H
#define	PGAS_DSL_H

#include "common.h"
#include <assert.h>

#define PGAS_DSL_MASK_BITS 25
#if __WORDSIZE == 32
#  define PGAS_DSL_ADDR_MASK        (0xFFFFFFFFL>>(32 - PGAS_DSL_MASK_BITS))
#  define PGAS_DSL_SIZE_NODE        (0x1L<<PGAS_DSL_MASK_BITS)
#else  /* !SCC */
#  define PGAS_DSL_ADDR_MASK        (0xFFFFFFFFFFFFFFFFL>>(64 - PGAS_DSL_MASK_BITS))
#  define PGAS_DSL_SIZE_NODE        (0x1L<<PGAS_DSL_MASK_BITS)
#endif	/* SCC */

extern void pgas_dsl_init();
extern void pgas_dsl_term();

extern int64_t pgas_dsl_read(uint64_t offset);
inline int64_t* pgas_dsl_readp(uint64_t offset);
extern int32_t pgas_dsl_read32(uint64_t offset);

extern void pgas_dsl_write(uint64_t offset, int64_t val);
extern void pgas_dsl_write32(uint64_t offset, int32_t val);

#endif	/* PGAS_DSL_H */

