/*
 *   File: tmN.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: simple TM benchmarks
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

/*
 * testing the assignement of addr to dsl cores
 */

#include <assert.h>

#include "tm2c.h"

#define SIS_SIZE 65536

size_t mem_size = SIS_SIZE * sizeof(int32_t);
size_t num_dsl = 16;

static inline nodeid_t
get_responsible_node(tm_intern_addr_t addr) 
{
#if defined(PGAS)
  return addr >> PGAS_DSL_MASK_BITS;
#else	 /* !PGAS */
#  if (ADDR_TO_DSL_SEL == 0)
  return hash_tw(addr >> ADDR_SHIFT_MASK) % num_dsl;
#  elif (ADDR_TO_DSL_SEL == 1)
  return ((addr) >> ADDR_SHIFT_MASK) % num_dsl;
#  endif
#endif	/* PGAS */
}

int
main(int argc, char **argv)
{
  TM2C_INIT;

  if (argc > 1)
    {
      mem_size = atoi(argv[1]) * sizeof(int32_t);
    }

  if (argc > 1)
    {
      num_dsl = atoi(argv[2]);
    }

  ONCE
    {
      printf("memory size in bytes: %lu, words: %lu\n  emulating %lu dsl nodes\n",
	     ((LU) mem_size), ((LU) mem_size / sizeof(int)), (LU) num_dsl);
    }
  int32_t *mem = (int32_t *) sys_shmalloc(SIS_SIZE * sizeof(int32_t));
  assert(mem != NULL);

  BARRIER;

  ONCE
    {
      PRINT("proc        | number of addr");

      size_t num_resp[TM2C_MAX_PROCS] = {0};

      uint32_t a;
      for (a = 0; a < SIS_SIZE; a++)
	{
	  tm_intern_addr_t ia = to_intern_addr(mem + a);
	  num_resp[get_responsible_node(ia)]++;
	}

      for (a = 0; a < TM2C_MAX_PROCS; a++)
	{
	  if (num_resp[a])
	    {
	      printf("%-20u%lu\n", a, (LU) num_resp[a]);
	    }
	}
    }

  BARRIER;

  sys_shfree((void*) mem);

  TM_END;

  EXIT(0);
}
