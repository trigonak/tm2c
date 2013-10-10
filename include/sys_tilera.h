/*
 *   File: sys_tilera.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: implementations of platform specific functions for
 *                Tilera
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

#ifndef SYS_TILERA_H
#define	SYS_TILERA_H

#include <tmc/alloc.h>
#include <tmc/cpus.h>
#include <tmc/task.h>
#include <tmc/udn.h>
#include <tmc/spin.h>
#include <tmc/sync.h>
#include <tmc/cmem.h>
#include <arch/cycle.h> 
#include <sys/time.h>

#include "common.h"
#include "tm2c_rpc.h"
#include <tmc/mem.h>

#define getticks get_cycle_count

#ifdef	__cplusplus
extern "C" {
#endif

  extern DynamicHeader *udn_header; //headers for tm2c_rpc
  extern tmc_sync_barrier_t *barrier_apps, *barrier_all; //BARRIERS

#define BARRIER tmc_sync_barrier_wait(barrier_apps); //app cores only
#define BARRIERW tmc_sync_barrier_wait(barrier_all); //all cores
#define BARRIER_DSL tmc_sync_barrier_wait(barrier_dsl); //all cores

#define getticks get_cycle_count

#define RCCE_num_ues TOTAL_NODES
#define RCCE_ue NODE_ID
#define RCCE_acquire_lock(a) 
#define RCCE_release_lock(a)
#define RCCE_init(a,b)
#define iRCCE_init(a)

#define t_vcharp sys_t_vcharp


  extern nodeid_t *dsl_nodes;
#define PS_BUFFER_SIZE 32

#if !defined(NOCM) && !defined(BACKOFF_RETRY) 			/* if any other CM (greedy, wholly, faircm) */
  extern int32_t **cm_abort_flags;
  extern int32_t *cm_abort_flag_mine;
#endif /* CM_H */

  extern DynamicHeader *udn_header; //headers for tm2c_rpc
  extern uint32_t* demux_tags;
  extern tmc_sync_barrier_t *barrier_apps, *barrier_all, *barrier_dsl; //BARRIERS


  INLINED nodeid_t
  NODE_ID(void) 
  {
    return (nodeid_t) TM2C_ID;
  }

  INLINED nodeid_t
  TOTAL_NODES(void) 
  {
    return (nodeid_t) NUM_UES;
  }

  INLINED tm_intern_addr_t
  to_intern_addr(tm_addr_t addr) {
#ifdef PGAS
    return fakemem_offset((void*) addr);
#else
    return ((tm_intern_addr_t) (addr));
#endif
  }

  INLINED tm_addr_t
  to_addr(tm_intern_addr_t i_addr) {
#ifdef PGAS
    return fakemem_addr_from_offset(i_addr);
#else
    /* return (tm_addr_t) ((uintptr_t) shmem_start_address + i_addr); */
    return (tm_addr_t) (i_addr);
#endif
  }

  INLINED int
  sys_sendcmd(void* data, size_t len, nodeid_t to)
  {
    tmc_udn_send_buffer(udn_header[to], UDN0_DEMUX_TAG, data, TM2C_RPC_REQ_SIZE_WORDS);
    /* tmc_udn_send_buffer(udn_header[to], UDN0_DEMUX_TAG, data, len/sizeof(int_reg_t)); */
    /* tmc_udn_send_buffer(udn_header[to], demux_tags[to], data, len/sizeof(int_reg_t)); */
    return 1;
  }

  INLINED int
  sys_sendcmd_no_sync(void* data, size_t len, nodeid_t to)
  {
    sys_sendcmd(data, len, to);
    return 1;
  }


  INLINED int
  sys_is_processed(nodeid_t to)
  {
    return 1;
  }

  INLINED int
  sys_sendcmd_all(void* data, size_t len)
  {
    int target;
    for (target = 0; target < NUM_DSL_NODES; target++)
      {
	tmc_udn_send_buffer(udn_header[dsl_nodes[target]], UDN0_DEMUX_TAG, data, TM2C_RPC_STATS_SIZE_WORDS);
	/* nodeid_t dsl_id = dsl_nodes[target]; */
	/* tmc_udn_send_buffer(udn_header[dsl_id], demux_tags[dsl_id], data, len/sizeof(int_reg_t)); */
      }
    return 1;
  
  }

  INLINED int
  sys_recvcmd(void* data, size_t len, nodeid_t from)
  {
    tmc_udn0_receive_buffer(data, TM2C_RPC_REPLY_SIZE_WORDS);

    /* switch (demux_tags[from]) */
    /*   { */
    /*   case UDN0_DEMUX_TAG: */
    /* 	tmc_udn0_receive_buffer(data, len/sizeof(int_reg_t)); */
    /* 	break; */
    /*   case UDN1_DEMUX_TAG: */
    /* 	tmc_udn1_receive_buffer(data, len/sizeof(int_reg_t)); */
    /* 	break; */
    /*   case UDN2_DEMUX_TAG: */
    /* 	tmc_udn2_receive_buffer(data, len/sizeof(int_reg_t)); */
    /* 	break; */
    /*   default:			/\* 3 *\/ */
    /* 	tmc_udn3_receive_buffer(data, len/sizeof(int_reg_t)); */
    /* 	break; */
    /*   } */
    return 1;
  }

  INLINED double
  wtime(void) 
  {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double) t.tv_sec + ((double) t.tv_usec) / 1000000.0;
  }


#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
  INLINED void
  abort_node(nodeid_t node, TM2C_CONFLICT_T reason)
  {
    while(arch_atomic_val_compare_and_exchange(cm_abort_flags[node], NO_CONFLICT, reason) == PERSISTING_WRITES)
      {
	cycle_relax();
      }
  }

  INLINED TM2C_CONFLICT_T
  check_aborted()
  {
    return (*cm_abort_flag_mine != NO_CONFLICT);
    /* return arch_atomic_exchange(cm_abort_flag_mine, NO_CONFLICT); */
  }

  INLINED TM2C_CONFLICT_T
  get_abort_reason()
  {
    return (*cm_abort_flag_mine);	
  }

  INLINED void
  set_tx_running()
  {
    /* arch_atomic_exchange(cm_abort_flag_mine, NO_CONFLICT); */
    *cm_abort_flag_mine = NO_CONFLICT;
    /* tmc_mem_fence(); */
  }

  INLINED void
  set_tx_committed()
  {
    /* arch_atomic_exchange(cm_abort_flag_mine, TX_COMMITTED); */
    *cm_abort_flag_mine = TX_COMMITTED;
  }

  INLINED TM2C_CONFLICT_T
  set_tx_persisting()
  {
    return arch_atomic_bool_compare_and_exchange(cm_abort_flag_mine, NO_CONFLICT, PERSISTING_WRITES);
  }
#endif	/* NOCM */


#ifdef	__cplusplus
}
#endif

#endif	/* SYS_TILERA_H */

