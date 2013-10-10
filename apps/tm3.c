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
 * Testing PGAS
 */

#include <assert.h>

#if !defined(DO_TIMINGS)
#  define DO_TIMINGS
#endif

#include "tm2c.h"

#define REPS 1000

int
main(int argc, char** argv)
{

  int steps = REPS<<5;

  TM2C_INIT;

  PF_MSG(0, "TX_LOAD");
  PF_MSG(1, "send & receive");
  PF_MSG(2, "send");
  PF_MSG(3, "receive");

  if (argc > 1)
    {
      steps = atoi(argv[1]);
    }

  int* sm = (int*) sys_shmalloc(steps * sizeof (int));

  BARRIER;
  if (argc < 3)
    { //ONLY 1 cores sending messages
      ONCE
	{
	  PRINT("profiling loads from a single process");
	}
      assert(REPS <= steps);

      ONCE
        {
	  int sum;

	  int rounds = 1;

	  while (REPS * rounds++ <= steps)
	    {
	      TX_START;
	      sum = 0;
	      int i;
	      for (i = 0; i < REPS; i++)
		{
		  int* addr = sm + i + (rounds * REPS);
		  PF_START(0);
		  sum += *(int*) TX_LOAD(addr);
		  PF_STOP(0);
		}
	      TX_COMMIT;
	    }
	  PRINT("sum -- %d", sum);
	  PF_PRINT;
	}
    }
  else
    { //ALL cores sending messages
      ONCE
	{
	  PRINT("profiling loads from all processes");
	}
      int sum = 0;

      int rounds = 1;

      while (REPS * rounds++ <= steps)
	{
	  TX_START;
	  sum = 0;
	  int i;
	  for (i = 0; i < REPS; i++)
	    {
	      int* addr = sm + ((i + (rounds * REPS) + TM2C_ID) % steps);
	      PF_START(0);
	      sum += *(int*) TX_LOAD(addr);
	      PF_STOP(0);
	    }
	  TX_COMMIT;
	}
      PRINT("sum -- %d", sum);
      PF_PRINT;
    }


  BARRIER;
  sys_shfree((void*) sm);

  TM_END;
  EXIT(0);
}
