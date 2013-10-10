/*
 *   File: tm2c.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: TM2C is the most portable software transactional
 *                memory (STM) to date. TM2C is distributed: its design
 *                is fully decentralized and builds on top of message
 *                passing. Nevertheless, TM2C targets large-scale
 *                many-cores, not distributed systems. Although
 *                decentralized (and therefore highly scalable),
 *                TM2C guarantees the termination of every
 *                transaction, unlike other distributed STMs.
 *
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
 * File:   tm2c.c
 * Author: trigonak
 *
 * Created on June 16, 2011, 12:29 PM
 */

#include <sys/param.h>

#include "tm2c.h"

nodeid_t ID;
nodeid_t NUM_UES;
nodeid_t NUM_DSL_NODES;
nodeid_t NUM_APP_NODES;

tm2c_tx_t *tm2c_tx = NULL;
tm2c_tx_node_t *tm2c_tx_node = NULL;

double duration__ = 0;

const char* conflict_reasons[4] = 
  {
    "NO_CONFLICT",
    "READ_AFTER_WRITE",
    "WRITE_AFTER_READ",
    "WRITE_AFTER_WRITE"
  };


//custom assignment
const unsigned short dsl_node_hex[8] = {0xAAAA,0xAAAA,0xAAAA,0xAAAA,0xAAAA,0xAAAA,0xAAAA,0xAAAA};
const uint8_t dsl_node[] =
  {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
  };


/*______________________________________________________________________________________________________
 * TM Interface                                                                                         |
 *______________________________________________________________________________________________________|
 */

int
is_app_core(int id)
{
#if (DSL_CORES_ASSIGN == 0)
  return (id % DSL_PER_NODE) != 0;
#elif (DSL_CORES_ASSIGN == 1)
  return !dsl_node[id];
#elif (DSL_CORES_ASSIGN == 2)
  return (buf[id/(8 * sizeof(unsigned short))]>>(id%(8 * sizeof(unsigned short))))&0x01;
#endif
}

int
is_dsl_core(int id)
{
  return !is_app_core(id);
}


void
tm2c_init()
{
  PF_MSG(9, "receiving");
  PF_MSG(10, "sending");

  sys_tm2c_init();
  if (!is_app_core(ID)) 
    {
      //dsl node
      tm2c_dsl_init();
    }
  else 
    { //app node
      tm2c_app_init();
      tm2c_tx_node = tm2c_tx_meta_node_new();
      tm2c_tx = tm2c_tx_meta_new();
      if (tm2c_tx == NULL || tm2c_tx_node == NULL) 
	{
	  PRINTD("Could not alloc tx metadata @ TM2C_INIT");
	  EXIT(-1);
	}
    }
}

void
tm2c_init_system(int* argc, char** argv[])
{
  /* calculate the getticks correction if DO_TIMINGS is set */
  PF_CORRECTION;

  /* call platform level initializer */
  sys_tm2c_init_system(argc, argv);

  /* initialize globals */
  ID            = NODE_ID();
  NUM_UES       = TOTAL_NODES();

  uint32_t i, tot = 0;
  for (i = 0; i < NUM_UES; i++) 
    {
      if (!is_app_core(i)) 
	{
	  tot++;
	}
    }
  NUM_DSL_NODES = tot;
  NUM_APP_NODES = NUM_UES - tot;

  tm2c_init_barrier();
}
/*
 * Trampolining code for terminating everything
 */
void
tm2c_term()
{
  if (!is_app_core(ID)) 
    {
      // DSL node
      // common stuff
      uint32_t c;
      for (c = 0; c < TOTAL_NODES(); c++)
	{
	  if (NODE_ID() == c)
	    {
#ifdef DO_TIMINGS
	      printf("(( %02d ))", c);
#endif
	      PF_PRINT;
	    }
	  BARRIER_DSL;
	}

      BARRIERW;
      // platform specific stuff
      sys_dsl_term();
#if defined(PGAS)
  nodeid_t j;
  for (j = 0; j < NUM_UES; j++)
    {
      if (is_app_core(j))
	{ /*only for non DSL cores*/
	  free(PGAS_write_sets[j]);
	}
    }

  free(PGAS_write_sets);
#endif

  tm2c_ht_free(tm2c_ht);

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
  free(cm_metadata_core);
#endif

    }
  else 
    { 
      //app node
      // common stuff
      BARRIERW;
      uint32_t c;
      for (c = 0; c < TOTAL_NODES(); c++)
	{
	  if (NODE_ID() == c)
	    {
#ifdef DO_TIMINGS
	      printf("(( %02d ))", c);
#endif
	      PF_PRINT;
	    }
	  BARRIER;
	}
      // plaftom specific stuff
      sys_app_term();

      free(tm2c_tx_node);
      free(tm2c_tx);

    }
}


#define PRINT_ON_ABORT()						\
  if (tm2c_tx->aborts == 1e4 || tm2c_tx->aborts == 1e5 || tm2c_tx->aborts == 1e6) \
    {									\
      PRINT("** number of aborts: %u", tm2c_tx->aborts);			\
    }

#define PRINT_ON_RETRY()						\
  if (tm2c_tx->retries == 1e3 || tm2c_tx->retries == 1e4 || tm2c_tx->retries == 1e5) \
    {									\
      PRINT("** number of retries: %u", tm2c_tx->retries);		\
    }


void 
tm2c_handle_abort(tm2c_tx_t* tm2c_tx, TM2C_CONFLICT_T reason) 
{
  tm2c_rpc_rls_all(reason);
  tm2c_tx->aborts++;
  
  /* PRINT_ON_ABORT(); */
  /* PRINT_ON_RETRY(); */

  /* switch (reason) */
  /*   { */
  /*   case READ_AFTER_WRITE: */
  /*     tm2c_tx->aborts_raw++; */
  /*     break; */
  /*   case WRITE_AFTER_READ: */
  /*     tm2c_tx->aborts_war++; */
  /*     break; */
  /*   case WRITE_AFTER_WRITE: */
  /*     tm2c_tx->aborts_waw++; */
  /*     break; */
  /*   default: */
  /*     /\* nothing *\/ */
  /*     break; */
  /*   } */

#if !defined(PGAS)
  write_set_empty(tm2c_tx->write_set);
#endif
  mem_info_on_abort(tm2c_tx->mem_info);
    
#if defined(BACKOFF_RETRY)
  /*BACKOFF and RETRY*/
  if (BACKOFF_MAX > 0)  
    {
      uint32_t wait_exp = tm2c_tx->retries;
      if (wait_exp > BACKOFF_MAX)
	{
	  wait_exp = BACKOFF_MAX;
	}

      uint32_t wait_max = BACKOFF_DELAY;
      wait_max <<= (wait_exp - 1);

      uint32_t wait = tm2c_rand() % wait_max;

      ndelay(wait);
    }

#else
  wait_cycles(50 * (tm2c_tx->retries & 0xFF));
#endif
}

void
tm2c_rpc_store_finish_all(unsigned int locked) 
{
#if !defined(PGAS)
  write_entry_t *we_current = tm2c_tx->write_set->write_entries;
#endif
  while (locked-- > 0) 
    {
#ifdef PGAS
      // tm2c_rpc_store_finish(we_current->address, we_current->value);
#else
      tm2c_rpc_store_rls(to_addr(we_current[locked].address));
#endif
    }
}

void
tm2c_rpc_store_all() 
{
#if !defined(PGAS)
  write_entry_t* write_entries = tm2c_tx->write_set->write_entries;
  uint32_t nb_entries = nb_entries = tm2c_tx->write_set->nb_entries;
  uint32_t locked = 0;
  while (locked < nb_entries) 
    {
      TM2C_CONFLICT_T conflict;
      tm_addr_t addr = to_addr(write_entries[locked].address);
      TXCHKABORTED();
      if ((conflict = tm2c_rpc_store(addr, 0)) != NO_CONFLICT)
	{
	  TX_ABORT(conflict);
	}
      locked++;
    }
#else
  PRINT(" *** warning : using tm2c_rpc_store_all with PGAS");
#endif
}

