/*
 *   File: sys_SCC_ssmp.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: implementations of platform specific functions for Intel SCC
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

#ifndef SYS_SCC_SSMP_H;
#define	SYS_SCC_SSMP_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <ssmp.h>

#include "common.h"
#include "tm2c_rpc.h"
#include "tm2c_sys.h"
#include "RCCE.h"

#define BARRIER     ssmp_barrier_wait(1);
#define BARRIERW    ssmp_barrier_wait(0);
#define BARRIER_DSL ssmp_barrier_wait(3);

  extern tm_addr_t shmem_start_address;
  extern nodeid_t *dsl_nodes;
  extern nodeid_t TM2C_ID;
  extern nodeid_t TM2C_NUM_NODES;

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
  extern ssmp_mpb_line_t *abort_reason_mine;
  extern ssmp_mpb_line_t **abort_reasons;
  extern volatile ssmp_mpb_line_t *persisting_mine;
  extern volatile ssmp_mpb_line_t **persisting;
  extern t_vcharp *cm_abort_flags;
  extern t_vcharp cm_abort_flag_mine;
  extern t_vcharp virtual_lockaddress[48];
#endif /* CM_H */

extern size_t pgas_app_addr_offs(void* addr);
extern void* pgas_app_addr_from_offs(size_t offs);

#define PS_BUFFER_SIZE 32
    
  EXINLINED nodeid_t
  NODE_ID(void)
  {
    return TM2C_ID;
  }

  INLINED nodeid_t
  TOTAL_NODES(void)
  {
    return TM2C_NUM_NODES;
  }

  INLINED tm_intern_addr_t
  to_intern_addr(tm_addr_t addr)
  {
#ifdef PGAS
  return pgas_app_addr_offs((void*) addr);
#else
    return ((tm_intern_addr_t)((uintptr_t)addr - (uintptr_t)shmem_start_address));
#endif
  }


  EXINLINED tm_addr_t
  to_addr(tm_intern_addr_t i_addr)
  {
#ifdef PGAS
    return pgas_app_addr_from_offs(i_addr);
#else
    return (tm_addr_t)((uintptr_t)shmem_start_address + i_addr);
#endif
  }

  INLINED int
  sys_sendcmd(void* data, size_t len, nodeid_t to)
  {
    ssmp_send(to, (ssmp_msg_t *) data);
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
    ssmp_recv_from(from, (ssmp_msg_t *) data, 8);
    return 1;
  }

  INLINED double
  wtime(void)
  {
    return RCCE_wtime();
  }


#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */

  /* 
     using the TST as follows:
     - tst set = tx is running
     - tst not set = tx not running (aborted, persisting, comitted)
  */


  INLINED void
  mpb_write_line(volatile ssmp_mpb_line_t *line, uint32_t val)
  {
    uint32_t w;
    for (w = 0; w < 8; w++)	/* flushing Write Combine Buffer */
      {
	line->words[w] = val;
      }
  }

  INLINED void
  abort_node(nodeid_t node, TM2C_CONFLICT_T reason) {
    mpb_write_line(abort_reasons[node], reason);
  
    *cm_abort_flags[node] = 0;
  
    MPB_INV();
    uint32_t was_persisting = 0;
    while (persisting[node]->words[0] == 1)
      {
	was_persisting = 1;
	MPB_INV();
	wait_cycles(80);
      }
  
    if (!was_persisting) 
      {
	*cm_abort_flags[node] = 0;
      }
  }

  INLINED TM2C_CONFLICT_T
  check_aborted() {
    return *cm_abort_flag_mine;
  }

  INLINED TM2C_CONFLICT_T
  get_abort_reason() {
    MPB_INV();
    return abort_reason_mine->words[0];
  }

  INLINED void
  set_tx_running() {
    *cm_abort_flag_mine & 0x1; 	/* set the flag */
  }

  INLINED void
  set_tx_committed() {
    mpb_write_line(persisting_mine, 0);
  }

  INLINED TM2C_CONFLICT_T
  set_tx_persisting() {
    mpb_write_line(persisting_mine, 1);
    if (*cm_abort_flag_mine) { 	/* was aborted */
      mpb_write_line(persisting_mine, 0);
      return 0;
    }

    return 1;
  }
#endif	/* NOCM */

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_SCC_SSMP_H */

