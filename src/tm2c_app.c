/*
 *   File: tm2c_app.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: main functionality for application processes
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
 * File:   tm2c_app.c
 * Author: trigonak
 *
 * Created on March 7, 2011, 4:45 PM
 * 
 */

#include <malloc.h>
#include "common.h"
#include "tm2c_app.h"
#include "tm2c.h"

#include <limits.h>
#include <assert.h>
#include "hash.h"
#ifdef PGAS
#  include "pgas_app.h"
#  include "pgas_dsl.h"
#endif

nodeid_t* dsl_nodes; /* holds the ids of the nodes. ids are in range 0..64 (possibly more)
			To get the address of the node, one must call id_to_addr */
unsigned short nodes_contacted[TM2C_MAX_PROCS];
TM2C_RPC_REQ *psc;
int64_t read_value;
unsigned long int* tm2c_rand_seeds;

static inline void tm2c_rpc_sendb(nodeid_t targ, TM2C_RPC_REQ_TYPE op, tm_intern_addr_t ad);
static inline void tm2c_rpc_sendbr(nodeid_t targ, TM2C_RPC_REQ_TYPE op, tm_intern_addr_t ad, TM2C_CONFLICT_T resp);
static inline void tm2c_rpc_sendbv(nodeid_t targ, TM2C_RPC_REQ_TYPE op, tm_intern_addr_t ad, int64_t va);
static inline TM2C_CONFLICT_T tm2c_rpc_recvb(nodeid_t from);
/*
 * Takes the local representation of the address, and finds the node
 * responsible for it.
 */
static inline nodeid_t get_responsible_node(tm_intern_addr_t addr);

unsigned long* 
seed_rand() 
{
  unsigned long* seeds;
  seeds = (unsigned long*) malloc(3 * sizeof(unsigned long));
  assert(seeds != NULL);
  seeds[0] = getticks() % 123456789;
  seeds[1] = getticks() % 362436069;
  seeds[2] = getticks() % 521288629;
  return seeds;
}


void
tm2c_app_init(void) 
{
  PRINTD("NUM_DSL_NODES = %d", NUM_DSL_NODES);
  if ((dsl_nodes = (unsigned int *) malloc(NUM_DSL_NODES * sizeof (unsigned int))) == NULL)
    {
      PRINT("malloc dsl_nodes");
      EXIT(-1);
    }

  psc = (TM2C_RPC_REQ*) memalign(CACHE_LINE_SIZE, sizeof (TM2C_RPC_REQ));
  if (psc == NULL)
    {
      PRINT("malloc psc == NULL");
    }

  int dsln = 0;
  unsigned int j;
  for (j = 0; j < NUM_UES; j++) 
    {
      nodes_contacted[j] = 0;
      if (!is_app_core(j)) 
	{
	  dsl_nodes[dsln++] = j;
	}
    }

  tm2c_rand_seeds = seed_rand();
  sys_app_init();
  /* PRINT("[APP NODE] Initialized TM2C.."); */
}

static inline void
tm2c_rpc_sendb(nodeid_t target, TM2C_RPC_REQ_TYPE command,
         tm_intern_addr_t address)
{
  psc->type = command;
#if defined(PLATFORM_TILERA)
  psc->nodeId = TM2C_ID;
#endif
  psc->address = address;

#if defined(WHOLLY)
  psc->tx_metadata = tm2c_tx_node->tx_committed;
#elif defined(FAIRCM)
  psc->tx_metadata = tm2c_tx_node->tx_duration;
#elif defined(GREEDY)
#  ifdef GREEDY_GLOBAL_TS
  psc->tx_metadata = tm2c_tx->start_ts;
#  else
  psc->tx_metadata = getticks() - tm2c_tx->start_ts;
#  endif
#endif

  sys_sendcmd(psc, sizeof(TM2C_RPC_REQ), target);
}

static inline void
tm2c_rpc_sendbr(nodeid_t target, TM2C_RPC_REQ_TYPE command,
	  tm_intern_addr_t address, TM2C_CONFLICT_T response)
{
  psc->type = command;
#if defined(PLATFORM_TILERA)
  psc->nodeId = TM2C_ID;
#endif
  psc->address = address;

#if defined(WHOLLY)
  psc->tx_metadata = tm2c_tx_node->tx_committed;
#elif defined(FAIRCM)
  psc->tx_metadata = tm2c_tx_node->tx_duration;
#elif defined(GREEDY)
#  ifdef GREEDY_GLOBAL_TS
  psc->tx_metadata = tm2c_tx->start_ts;
#  else
  psc->tx_metadata = getticks() - tm2c_tx->start_ts;
#  endif
#endif

#if defined(PGAS)
  psc->response = response;
#endif	/* PGAS */
#if defined(SSMP_NO_SYNC_RESP)
  sys_sendcmd_no_sync(psc, sizeof (TM2C_RPC_REQ), target);
#else
  sys_sendcmd(psc, sizeof (TM2C_RPC_REQ), target);
#endif
}

static inline int
tm2c_rpc_is_processed(nodeid_t target)
{
  return sys_is_processed(target);
}

static inline void
tm2c_rpc_sendbv(nodeid_t target, TM2C_RPC_REQ_TYPE command,
	  tm_intern_addr_t address, int64_t value)
{
  psc->type = command;
#if defined(PLATFORM_TILERA)
  psc->nodeId = TM2C_ID;
#endif
  psc->address = address;
    
#if defined(WHOLLY)
  psc->tx_metadata = tm2c_tx_node->tx_committed;
#elif defined(FAIRCM)
  psc->tx_metadata = tm2c_tx_node->tx_duration;
#elif defined(GREEDY)
#  ifdef GREEDY_GLOBAL_TS
  psc->tx_metadata = tm2c_tx->start_ts;
#  else
  psc->tx_metadata = getticks() - tm2c_tx->start_ts;
#  endif
#endif

#if defined(PGAS)
  psc->write_value = value;
#endif	/* PGAS */
  sys_sendcmd(psc, sizeof(TM2C_RPC_REQ), target);
}

static inline TM2C_CONFLICT_T
tm2c_rpc_recvb(nodeid_t from)
{
  TM2C_RPC_REPLY cmd;
  sys_recvcmd(&cmd, sizeof (TM2C_RPC_REPLY), from);

#ifdef PGAS
  read_value = cmd.value;
#endif
  return cmd.response;
}

/*
 * ____________________________________________________________________________________________
 TM interface _________________________________________________________________________________|
 * ____________________________________________________________________________________________|
 */

TM2C_CONFLICT_T
tm2c_rpc_load(tm_addr_t address, int words) 
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);

  nodeid_t responsible_node_seq = get_responsible_node(intern_addr);
  nodes_contacted[responsible_node_seq]++;

  nodeid_t responsible_node = dsl_nodes[responsible_node_seq];

#ifdef PGAS
  intern_addr &= PGAS_DSL_ADDR_MASK;
  tm2c_rpc_sendbv(responsible_node, TM2C_RPC_LOAD, intern_addr, words);
#else
  tm2c_rpc_sendb(responsible_node, TM2C_RPC_LOAD, intern_addr);
#endif

  TM2C_CONFLICT_T response = tm2c_rpc_recvb(responsible_node);
  if (response != NO_CONFLICT)
    {
      nodes_contacted[responsible_node_seq] = 0;
    }

  return response;
}


/* value is used only on PGAS */
TM2C_CONFLICT_T
tm2c_rpc_store(tm_addr_t address, int64_t value)
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);
  nodeid_t responsible_node_seq = get_responsible_node(intern_addr);

  nodes_contacted[responsible_node_seq]++;
  nodeid_t responsible_node = dsl_nodes[responsible_node_seq];

#ifdef PGAS
  intern_addr &= PGAS_DSL_ADDR_MASK;
  tm2c_rpc_sendbv(responsible_node, TM2C_RPC_STORE, intern_addr, value);
#else
  tm2c_rpc_sendb(responsible_node, TM2C_RPC_STORE, intern_addr); //make sync
#endif

  TM2C_CONFLICT_T response = tm2c_rpc_recvb(responsible_node);
  if (response != NO_CONFLICT)
    {
      nodes_contacted[responsible_node_seq] = 0;
    }

  return response;
}

#ifdef PGAS

TM2C_CONFLICT_T
tm2c_rpc_store_inc(tm_addr_t address, int64_t increment) 
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);
  nodeid_t responsible_node_seq = get_responsible_node(intern_addr);

  nodes_contacted[responsible_node_seq]++;
  nodeid_t responsible_node = dsl_nodes[responsible_node_seq];

  intern_addr &= PGAS_DSL_ADDR_MASK;
  tm2c_rpc_sendbv(responsible_node, TM2C_RPC_STORE_INC, intern_addr, increment);

  TM2C_CONFLICT_T response = tm2c_rpc_recvb(responsible_node);
  if (response != NO_CONFLICT)
    {
      nodes_contacted[responsible_node_seq] = 0;
    }

  return response;
}
#endif	/* PGAS */

uint64_t
tm2c_rpc_notx_load(tm_addr_t address, int words) 
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);
  nodeid_t responsible_node = get_responsible_node(intern_addr);
  responsible_node = dsl_nodes[responsible_node];

#if defined(PGAS)
  intern_addr &= PGAS_DSL_ADDR_MASK;
#endif	/* PGAS */

  tm2c_rpc_sendbr(responsible_node, TM2C_RPC_LOAD_NONTX, intern_addr, words);
  tm2c_rpc_recvb(responsible_node);

  return read_value;
}

void
tm2c_rpc_notx_store(tm_addr_t address, int64_t value) 
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);
  nodeid_t responsible_node = get_responsible_node(intern_addr);
  responsible_node = dsl_nodes[responsible_node];

#if defined(PGAS)
  intern_addr &= PGAS_DSL_ADDR_MASK;
#endif	/* PGAS */

  tm2c_rpc_sendbv(responsible_node, TM2C_RPC_STORE_NONTX, intern_addr, value);
}

void
tm2c_rpc_load_rls(tm_addr_t address) 
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);
  nodeid_t responsible_node = get_responsible_node(intern_addr);
#if defined(PGAS)
  intern_addr &= PGAS_DSL_ADDR_MASK;
#endif	/* PGAS */

  nodes_contacted[responsible_node]--;
  responsible_node = dsl_nodes[responsible_node];

  tm2c_rpc_sendb(responsible_node, TM2C_RPC_LOAD_RLS, intern_addr);
}

void
tm2c_rpc_store_rls(tm_addr_t address) 
{
  tm_intern_addr_t intern_addr = to_intern_addr(address);
  nodeid_t responsible_node = get_responsible_node(intern_addr);

#if defined(PGAS)
  intern_addr &= PGAS_DSL_ADDR_MASK;
#endif	/* PGAS */

  nodes_contacted[responsible_node]--;
  responsible_node = dsl_nodes[responsible_node];

  tm2c_rpc_sendb(responsible_node, TM2C_RPC_STORE_FINISH, intern_addr);
}

void 
tm2c_rpc_rls_all(TM2C_CONFLICT_T conflict) 
{
#if defined(PGAS)
  nodeid_t i;
  for (i = 0; i < NUM_DSL_NODES; i++) 
    {
      if (nodes_contacted[i] > 0) 
	{ 
	  tm2c_rpc_sendbr(dsl_nodes[i], TM2C_RPC_RMV_NODE, 0, conflict);
	}
    }

  int all_processed = 0;
  while (!all_processed)
    {
      all_processed = 1;
      for (i = 0; i < NUM_DSL_NODES; i++) 
	{
	  if (nodes_contacted[i] > 0) 
	    { 
	      if (tm2c_rpc_is_processed(dsl_nodes[i]))
		{
		  nodes_contacted[i] = 0;
		}
	      else
		{
		  all_processed = 0;
		}
	    }
	}
    }
#else
  nodeid_t i;
  for (i = 0; i < NUM_DSL_NODES; i++) 
    {
      if (nodes_contacted[i] > 0) 
	{ 
	  tm2c_rpc_sendbr(dsl_nodes[i], TM2C_RPC_RMV_NODE, 0, conflict);
	  nodes_contacted[i] = 0;
	}
    }
#endif
}

void
tm2c_rpc_stats(tm2c_tx_node_t* stats, double duration)
{
  TM2C_RPC_STATS_T* stats_cmd = (TM2C_RPC_STATS_T*) malloc(64);
  if (stats_cmd == NULL)
    {
      PRINT("malloc @ tm2c_rpc_send_stats");
      stats_cmd = (TM2C_RPC_STATS_T*) psc;
    }

  if (duration == 0)
    {
      duration = 1;		/* to make stats work properly */
    }

  stats_cmd->type = TM2C_RPC_STATS;
  stats_cmd->nodeId = NODE_ID();

  stats_cmd->aborts = stats->tx_aborted;
  stats_cmd->commits = stats->tx_committed;
  stats_cmd->max_retries = stats->max_retries;
  stats_cmd->tx_duration = duration;

  sys_sendcmd_all(stats_cmd, sizeof(TM2C_RPC_STATS_T));

  stats_cmd->aborts_raw = stats->aborts_raw;
  stats_cmd->aborts_war = stats->aborts_war;
  stats_cmd->aborts_waw = stats->aborts_waw;
  stats_cmd->tx_duration = 0;
    
  sys_sendcmd_all(stats_cmd, sizeof(TM2C_RPC_STATS_T));

  BARRIERW;
  free(stats_cmd);
}

TM2C_CONFLICT_T
tm2c_rpc_dummy(nodeid_t node)
{
  node = dsl_nodes[node];
  tm2c_rpc_sendb(node, TM2C_RPC_UKNOWN, 0);
  TM2C_CONFLICT_T response = tm2c_rpc_recvb(node);
  return response;
}

static inline nodeid_t
get_responsible_node(tm_intern_addr_t addr) 
{
#if defined(PGAS)
  return addr >> PGAS_DSL_MASK_BITS;
#else	 /* !PGAS */
#  if (ADDR_TO_DSL_SEL == 0)
  return hash_tw(addr >> ADDR_SHIFT_MASK) % NUM_DSL_NODES;
#  elif (ADDR_TO_DSL_SEL == 1)
  return ((addr) >> ADDR_SHIFT_MASK) % NUM_DSL_NODES;
#  endif
#endif	/* PGAS */
}
