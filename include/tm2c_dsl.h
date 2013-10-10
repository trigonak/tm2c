/*
 *   File: tm2c_dsl.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: TM2C service functions
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

#ifndef TM2C_DSL_H
#define TM2C_DSL_H

#include "common.h"
#include "tm2c_app.h"
#include "tm2c_dsl_ht.h"
#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
#include "tm2c_cm.h"
#endif

#ifdef PGAS
extern tm2c_write_set_pgas_t **PGAS_write_sets;
#endif

void tm2c_dsl_init(void);

void dsl_service();

INLINED TM2C_CONFLICT_T try_load(nodeid_t nodeId, tm_intern_addr_t tm_address);
INLINED TM2C_CONFLICT_T try_store(nodeid_t nodeId, tm_intern_addr_t tm_address);

void tm2c_dsl_print_global_stats();
void print_hashtable_usage();

extern unsigned long int tm2c_stats_total,
  tm2c_stats_commits,
  tm2c_stats_aborts,
  tm2c_stats_max_retries,
  tm2c_stats_aborts_war,
  tm2c_stats_aborts_raw,
  tm2c_stats_aborts_waw,
  tm2c_stats_received;
extern double tm2c_stats_duration;

extern tm2c_ht_t tm2c_ht;

INLINED TM2C_CONFLICT_T
try_load(nodeid_t nodeId, tm_intern_addr_t tm_address) 
{
  return tm2c_ht_insert(tm2c_ht, nodeId, tm_address, READ);
}

INLINED TM2C_CONFLICT_T
try_store(nodeid_t nodeId, tm_intern_addr_t tm_address) 
{
  return tm2c_ht_insert(tm2c_ht, nodeId, tm_address, WRITE);
}

INLINED void
load_rls(nodeid_t nodeId, tm_intern_addr_t tm_address)
{
  tm2c_ht_delete(tm2c_ht, nodeId, tm_address, READ);
}

INLINED void
store_rls(nodeid_t nodeId, tm_intern_addr_t tm_address)
{
  tm2c_ht_delete(tm2c_ht, nodeId, tm_address, WRITE);
}

#endif
