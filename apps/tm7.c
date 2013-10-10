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
 * a simple randomized workloads generator
 */

#include "tm2c.h"

/* DEFINES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define SHMEM_SIZE1     SIS_SIZE
#define NUM_TXOPS       100
#define UPDTX_PRCNT     0
#define WRITE_PRCNT     0
#define DURATION        1

#define ROLL(prcntg)    if (rand_range(100) <= (prcntg))
#define LOST            else


inline void update_tx(int* sis);
inline void ro_tx(int* sis);


/* GLOBALS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

unsigned int SIS_SIZE = 200;
unsigned int store_me, TM2C_ID;
int sum = 0;

int
main(int argc, char** argv)
{

  TM2C_INIT;

  srand_core();
  store_me = TM2C_ID;

  if (argc > 1) {
    SIS_SIZE = atoi(argv[1]);
  }

  int *sis = (int *) sys_shmalloc(SIS_SIZE * sizeof (int));
  if (sis == NULL)
    {
      PRINT("Error: sys_shmalloc failed");
      EXIT(-1);
    }

  int i;
  for (i = TM2C_ID; i < SIS_SIZE; i += NUM_UES)
    {
      NONTX_STORE(&sis[i], -1, TYPE_INT);
    }

  BARRIER;

  int txupdate = 0;
  int txro = 0;

  FOR(DURATION)
  { //seconds

    TX_START;

    ROLL(UPDTX_PRCNT)
    {
      //update tx
      txupdate++;
      update_tx(sis);
    }
    LOST
      {
	txro++;
	//read-only tx
	ro_tx(sis);
      }

    TX_COMMIT;
  }
  END_FOR;

  ONCE
    {
      printf("#id #pr #commit #com/s  #latency\n");
      FLUSH;
    }
  BARRIER;

  PRINT("%02d\t%d\t%d\t%f", NUM_UES,
	tm2c_tx_node->tx_committed,
	(int) (tm2c_tx_node->tx_committed / duration__),
	1000 * (duration__ / tm2c_tx_node->tx_committed));

  BARRIER;

  sys_shfree((void*) sis);

  TM_END;

  fprintf(stderr, "%d", sum);

  EXIT(0);
}

/*
 * Operations executed for a read-only Tx
 */
inline void
ro_tx(int* sis)
{
  int i;
  for (i = 0; i < NUM_TXOPS; i++)
    {
      long rnd = rand_range(SHMEM_SIZE1);
#ifdef PGAS
      sum = TX_LOAD(sis + rnd, TYPE_INT);
#else
      int *j = (int *) TX_LOAD(sis + rnd);
      sum = *j;
#endif
    }
}

/*
 * Operations executed for an update Tx
 */
inline void
update_tx(int* sis)
{
  int i;
  for (i = 0; i < NUM_TXOPS; i++)
    {
      long rnd = rand_range(SHMEM_SIZE1);

      ROLL(WRITE_PRCNT)
      {
	TX_STORE(sis + rnd, TM2C_ID, TYPE_INT);
      }
      LOST
        {
	  ro_tx(sis);
        }
    }
}
