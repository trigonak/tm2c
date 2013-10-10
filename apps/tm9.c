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
 * TX with WAR conflicts
 */

#include "tm2c.h"

#define SIS_SIZE 1000

int
main(int argc, char **argv)
{    
  PRINT("testing WAR conflicts");
  TM2C_INIT;
    
  int reps = SIS_SIZE<<4;
    
  if (argc > 1)
    {
      reps = atoi(argv[1]);
    }
    
  ONCE
    {
      PRINT("Repetitions %d", reps);
    }
    
  int* j = (int*) sys_shmalloc(reps * sizeof(int));
  if (j == NULL)
    {
      perror("sys_shmalloc\n");
    }

  BARRIER;
    
  /*
   * Write after read conflicts : some writers and some readers on the whole
   * memory
   */
    
  TX_START;
    
  if (TM2C_ID == min_app_id())
    {
      int i;
      for (i = 0; i < reps; i++)
	{
	  TX_LOAD(j + i);
	}
    }
  else
    {
      int i;
      for (i = 0; i < reps; i++)
	{
	  TX_STORE(j + i, TM2C_ID, TYPE_INT);
	}
    }


  TX_COMMIT;
    
  BARRIER;

  sys_shfree((void*) j);

  TM_END;

  EXIT(0);
}
