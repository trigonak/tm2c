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
 * TX with WAR conflicts -> TX with RAW conflicts
 */

#include <assert.h>

#if !defined(DO_TIMINGS)
#  define DO_TIMINGS
#endif

#include "tm2c.h"

#define SIS_SIZE 480

int
main(int argc, char **argv)
{

  TM2C_INIT;

  int *sis = (int *) sys_shmalloc(SIS_SIZE * sizeof (int));
  if (sis == NULL)
    {
      perror("sys_shmalloc");
    }

  int i;
  for (i = TM2C_ID; i < SIS_SIZE; i += NUM_UES)
    {
      sis[i] = -1;
    }

  PF_MSG(0, "write after read");
  PF_MSG(1, "read after write");

  BARRIER;

  PF_START(0);
  TX_START;
  int i;
  for (i = 0; i < SIS_SIZE; i++)
    {
      if (TM2C_ID == 1)
	{
	  TX_LOAD(sis + i);
	}
      else
	{
	  TX_STORE(sis + i, TM2C_ID, TYPE_INT);
	}
    }

  TX_COMMIT;
  PF_STOP(0);

  BARRIER;
    

  PF_START(1);
  TX_START;
  if(TM2C_ID == 1)
    {
      udelay(2500);
    }
  int i;
  for (i = 0; i < SIS_SIZE; i++)
    {
      if (TM2C_ID == 1)
	{
	  TX_LOAD(sis + i);
	}
      else
	{
	  TX_STORE(sis + i, TM2C_ID, TYPE_INT);
	}
    }

  TX_COMMIT;
  PF_STOP(1);
    
  sys_shfree((void*) sis);

  APP_EXEC_ORDER
    {
      PF_PRINT;
    }
  APP_EXEC_ORDER_END;

  TM_END;

  EXIT(0);
}
