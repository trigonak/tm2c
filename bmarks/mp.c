/*
 *   File: mp.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: testing the message passing latencies
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

#include <assert.h>

#if !defined(DO_TIMINGS)
#  define DO_TIMINGS
#endif
#include "tm2c.h"

#define REPS 1000000


int
main(int argc, char **argv) 
{
  PF_MSG(0, "roundtrip message");

  long long int steps = REPS;

  PF_MSG(0, "round-trip messaging latencies");

  TM2C_INIT;

#if !defined(NOCM) && !defined(BACKOFF_RETRY)
  ONCE
    {
      PRINT("*** better run mp with either NOCM and BACKOFF_RETRY to minimize the overhead!\n");
    }
#endif

  if (argc > 1) 
    {
      steps = atoll(argv[1]);
    }


#ifdef PGAS
  int *sm = (int *) sys_shmalloc(steps * sizeof (int));
#endif

  ONCE 
    {
      PRINT("## sending %lld messages", steps);
    }

  BARRIER;

  long long int rounds, sum = 0;

  ticks __start_ticks = getticks();
  nodeid_t to = 0;
  /* PRINT("sending to %d", to);  */

  PF_MSG(3, "Overall time");


  PF_START(3);
  for (rounds = 0; rounds < steps; rounds++) 
    {

#ifdef PGAS
      sum += (int) NONTX_LOAD(sm + rounds, 1);
#else
      DUMMY_MSG(to);
#endif

      to++;
      if (to == NUM_DSL_NODES)
      	{
      	  to = 0;
      	}
    }
  PF_STOP(3);
  ticks __end_ticks = getticks();
  ticks __duration_ticks = __end_ticks - __start_ticks;
  ticks __ticks_per_sec = (ticks) (1e9 * REF_SPEED_GHZ);
  duration__ = (double) __duration_ticks / __ticks_per_sec;

  if (sum > 0)
    {
      PRINT("sum -- %lld", sum);
    }

  total_samples[3] = steps;
  tm2c_tx_node->tx_committed = steps;

  TM_END;

  BARRIER;

  /* uint32_t c; */
  /* for (c = 0; c < TOTAL_NODES(); c++) */
  /*   { */
  /* 	if (NODE_ID() == c) */
  /* 	  { */
  /* 	    printf("(( %02d ))", c); */
  /* 	    PF_PRINT; */
  /* 	  } */
  /* 	BARRIER; */
  /*   } */

  EXIT(0);
}
