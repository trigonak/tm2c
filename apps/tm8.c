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
#include "tm2c.h"

#define SIS_SIZE 48


int
main(int argc, char** argv)
{    
  PRINT("testing WAR and RAW conflicts");

  TM2C_INIT;
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
    
  /*
   * Write after read conflicts : some writers and some readers on the whole
   * memory
   */
    
  TX_START;
    
  if (!TM2C_ID)
    {
      udelay(100);
    }
    
  int i;
  for (i = 2; i < SIS_SIZE; i++)
    {
      if (TM2C_ID >= 1)
	{
	  TX_LOAD(sis + i);
	}
      else
	{
	  TX_STORE(sis + i, TM2C_ID, TYPE_INT);
	}
    }

  TX_COMMIT;
    
  BARRIER;
    
  /*
   * Write after read conflicts : some writers and some readers on the whole
   * memory
   */
    
  TX_START;
    
  if (!TM2C_ID)
    {
      udelay(100);
    }
    
  int i;
  for (i = 0; i < SIS_SIZE; i++)
    {
      if (TM2C_ID >= 1)
	{
	  TX_LOAD(sis + i);
	}
      else
	{
	  TX_STORE(sis + i, TM2C_ID, TYPE_INT);
	}
    }

  TX_COMMIT;
    
  BARRIER;
    
  sys_shfree((void*) sis);

  TM_END;

  EXIT(0);
}
