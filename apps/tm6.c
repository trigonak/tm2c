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
 * Benchmarking reads in read-only TXs (1 pass: no use of read-buffering) -> 
 * -> read-only TXs (2 passes: the second pass is all from read set
 */

#if !defined(DO_TIMINGS)
#  define DO_TIMINGS
#endif

#include "tm2c.h"

int
main(int argc, char **argv)
{
  unsigned int SIS_SIZE = 4800;
    
  TM2C_INIT
    
    if (argc > 1)
      {
	SIS_SIZE = atoi(argv[1]);
      }

  int *sis = (int *) sys_shmalloc(SIS_SIZE * sizeof (int));
  if (sis == NULL)
    {
      perror("sys_shmalloc\n");
    }

  int i;
  for (i = TM2C_ID; i < SIS_SIZE; i += NUM_UES)
    {
      sis[i] = -1;
    }

  BARRIER;
    
  volatile size_t sum = 0;

  /*
   * Benchmark SIS_SIZE reads
   */
  TX_START;
  int i;
  for (i = 0; i < SIS_SIZE; i++)
    {
      PF_START(0);
      int j = *(int *) TX_LOAD(sis + i);
      PF_STOP(0);
      sum += j;
    }
  TX_COMMIT;
    
  if (sum == (13<<23))
    {
      PRINT("dummy print");
    }

  PF_MSG(0, "TX read");
  APP_EXEC_ORDER
    {
      PF_PRINT;
    }
  APP_EXEC_ORDER_END;

  BARRIER;
    
  sys_shfree((void*) sis);
  
  TM_END;

  EXIT(0);
}
