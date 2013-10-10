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
 * A write non-conflicting TX -> a read-only (whole mem) TX -> print results for validation
 */

#include "tm2c.h"

#define SIS_SIZE 1000
int
main(int argc, char **argv)
{
  PRINT("Write a whole array, then read the whole array repeatedly!");

  TM2C_INIT;

  int *sis = (int *) sys_shmalloc(SIS_SIZE * sizeof (int));
  if (sis == NULL)
    {
      perror("sys_shmalloc");
    }

  BARRIER;

  int reps = 1;

  FOR(1)
    {
      BARRIER;	     /* so that the sum vs. sum1 check makes sense! */

      size_t sum;

      TX_START;

      sum = 0;

      int i;
      for (i = 0; i < SIS_SIZE; i++)
	{
	  sum += reps + i;
	  TX_STORE(sis + i, reps + i, TYPE_INT);
	}

      TX_COMMIT;

      size_t sum2;

      TX_START;
      sum2 = 0;
      int i;
      int s[SIS_SIZE];
      for (i = 0; i < SIS_SIZE; i++)
	{
	  s[i] = *(int *) TX_LOAD(sis + i);
	  sum2 += s[i];
	}


      TX_COMMIT;

      if (sum != sum2)
	{
	  PRINT("sum (%lu) != sum2 (%lu)", (LU) sum, (LU) sum2);
	}
      reps++;

    }
  END_FOR;

  TM_END;

  EXIT(0);
}
