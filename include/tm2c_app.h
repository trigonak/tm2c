/*
 *   File: tm2c_app.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: TM2C RPC interface 
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
 * File:   tm2c_app.h
 * Author: trigonak
 *
 * Created on March 7, 2011, 10:50 AM
 * 
 */

#ifndef PUBSUBTM_H
#define	PUBSUBTM_H

#include "common.h"
#include "tm2c_tx_meta.h"
#include "tm2c_app.h"
#include "tm2c_rpc.h"

#ifdef	__cplusplus
extern "C" {
#endif

  //TODO: remove ? have them at .c file
  extern int64_t read_value;
  extern nodeid_t* dsl_nodes;
  extern unsigned long int* tm2c_rand_seeds;

  void tm2c_app_init(void);

  /* Try to subscribe the TX for reading the address
   */
  TM2C_CONFLICT_T tm2c_rpc_load(tm_addr_t address, int words);

  /* Try to publish a write on the address
   * XXX: try to unify the interface
   */
#ifdef PGAS
  TM2C_CONFLICT_T tm2c_rpc_store_inc(tm_addr_t address, int64_t increment);
#endif
  TM2C_CONFLICT_T tm2c_rpc_store(tm_addr_t address, int64_t value);

  /* Non-transactional read of 1 or 2 words from an address */
  uint64_t tm2c_rpc_notx_load(tm_addr_t address, int words);

  /* Non-transactional write to an address */
  void tm2c_rpc_notx_store(tm_addr_t address, int64_t value);

  /* Load_Rlss the TX from the address
   */
  void tm2c_rpc_load_rls(tm_addr_t address);
  /* Notifies the pub-sub that the publishing is done, so the value
   * is written on the shared mem
   */
  void tm2c_rpc_store_rls(tm_addr_t address);

  void tm2c_rpc_rls_all(TM2C_CONFLICT_T conflict);

  void tm2c_rpc_stats(tm2c_tx_node_t* tm2c_tx_node, double duration);

  /* Acquires all writes that are buffered in the write log of the
   * transaction
   */
  void tm2c_rpc_store_all(void);

  TM2C_CONFLICT_T tm2c_rpc_dummy(nodeid_t node);

#ifdef	__cplusplus
}
#endif

#endif	/* PUBSUBTM_H */
