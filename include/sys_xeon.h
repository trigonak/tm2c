/*
 *   File: sys_xeon.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: implementations of platform specific functions for Intel
 *                Xeon
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

#ifndef _SYS_XEON_H_
#define _SYS_XEON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <ssmp.h>

#include "common.h"
#include "tm2c_rpc.h"
#include "tm2c_malloc.h"

#ifdef PGAS
#  include "pgas_app.h"
#  include "pgas_dsl.h"
#endif


#define BARRIER  ssmp_barrier_wait(1);
#define BARRIERW ssmp_barrier_wait(0);
#define BARRIER_DSL ssmp_barrier_wait(14);

extern nodeid_t TM2C_ID;
extern nodeid_t TM2C_NUM_NODES;

extern TM2C_RPC_REPLY* tm2c_rpc_remote_msg; // holds the received msg
extern nodeid_t *dsl_nodes;

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
extern int32_t **cm_abort_flags;
extern int32_t *cm_abort_flag_mine;
#endif /* CM_H */

extern size_t pgas_app_addr_offs(void* addr);

/* --- inlined methods --- */
INLINED nodeid_t
NODE_ID(void)
{
  return TM2C_ID;
}


extern int is_app_core(int id);

INLINED nodeid_t
TOTAL_NODES(void)
{
  return TM2C_NUM_NODES;
}

INLINED int
sys_sendcmd(void* data, size_t len, nodeid_t to)
{
  ssmp_send(to, (ssmp_msg_t *) data);
  return 1;
}

INLINED int
sys_is_processed(nodeid_t to)
{
  return ssmp_send_is_free(to);
}

INLINED int
sys_sendcmd_no_sync(void* data, size_t len, nodeid_t to)
{
  ssmp_send_no_sync(to, (ssmp_msg_t *) data);
  return 1;
}

INLINED int
sys_sendcmd_all(void* data, size_t len)
{
  int target;
  for (target = 0; target < NUM_DSL_NODES; target++) 
    {
      ssmp_send(dsl_nodes[target], (ssmp_msg_t *) data);
    }
  return 1;
}

INLINED int
sys_recvcmd(void* data, size_t len, nodeid_t from)
{
  ssmp_recv_from(from, (ssmp_msg_t *) data);
  return 1;
}

INLINED tm_intern_addr_t
to_intern_addr(tm_addr_t addr)
{
#ifdef PGAS
  return pgas_app_addr_offs((void*) addr);
#else
  return (tm_intern_addr_t)addr;
#endif
}

INLINED tm_addr_t
to_addr(tm_intern_addr_t i_addr)
{
#ifdef PGAS
#else
  return (tm_addr_t)i_addr;
#endif
}

INLINED double
wtime(void)
{
  struct timeval t;
  gettimeofday(&t,NULL);
  return (double)t.tv_sec + ((double)t.tv_usec)/1000000.0;
}


#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
INLINED void
abort_node(nodeid_t node, TM2C_CONFLICT_T reason)
{
  while (__sync_val_compare_and_swap(cm_abort_flags[node], NO_CONFLICT, reason) == PERSISTING_WRITES) 
    {
      wait_cycles(180); 
    }
}

INLINED int
abort_node_is_persisting(nodeid_t node, TM2C_CONFLICT_T reason) 
{
  return (__sync_val_compare_and_swap(cm_abort_flags[node], NO_CONFLICT, reason) == PERSISTING_WRITES);
}

INLINED TM2C_CONFLICT_T
check_aborted() 
{
  return (*cm_abort_flag_mine != NO_CONFLICT);
}

INLINED TM2C_CONFLICT_T
get_abort_reason() 
{
  return (*cm_abort_flag_mine);	
}

INLINED void
set_tx_running() 
{
  *cm_abort_flag_mine = NO_CONFLICT;
}

INLINED void
set_tx_committed() 
{
  *cm_abort_flag_mine = TX_COMMITTED;
}

INLINED TM2C_CONFLICT_T
set_tx_persisting() 
{
  return __sync_bool_compare_and_swap(cm_abort_flag_mine, NO_CONFLICT, PERSISTING_WRITES);
}
#endif	/* NOCM */

#endif
