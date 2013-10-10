/*
 *   File: .c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: the generic interface for the internal hash table in TM2C
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

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include "tm2c_dsl_ht.h"
#include "hash.h"
#include "common.h"

  /*
   * ===========================================================================
   * ===================== The implementation part =============================
   * ===========================================================================
   */

#if USE_HASHTABLE_SSHT /************************************************************* SSHT ***/

  ssht_log_set_t** logs;

  static inline uint32_t
  tm2c_ht_get_hash(uintptr_t address)
  {
    return hash_tw((address>>3));
  }


  tm2c_ht_t
  tm2c_ht_new()
  {
    uint32_t i;
    logs = (ssht_log_set_t**) malloc(NUM_UES * sizeof(ssht_log_set_t*));
    assert(logs != NULL);

    for (i=0; i < NUM_UES; i++)
      {
	if (is_app_core(i))
	  {
	    logs[i] = ssht_log_set_new();
	  }
      }

    return ssht_new();
  }

  void
  tm2c_ht_free(tm2c_ht_t ht)
  {
    uint32_t i;
    for (i=0; i < NUM_UES; i++)
      {
	if (is_app_core(i))
	  {
	    free(logs[i]);
	  }
      }

    ssht_free((ssht_hashtable_t*) ht);
  }

  inline TM2C_CONFLICT_T
  tm2c_ht_insert(tm2c_ht_t tm2c_ht, nodeid_t node_id,
		      tm_intern_addr_t address, RW rw)
  {
#ifdef PGAS
    uint32_t bu = (address) & NUM_OF_BUCKETS_2;
#else  /* !PGAS */
    uint32_t bu = tm2c_ht_get_hash(address) & NUM_OF_BUCKETS_2;
#endif	/* PGAS */

    return ssht_insert(tm2c_ht, bu, logs[node_id], node_id,  address, rw);
  }

  inline void
  tm2c_ht_delete(tm2c_ht_t tm2c_ht, nodeid_t node_id, tm_intern_addr_t address, RW rw)
  {
    ssht_remove(logs[node_id], node_id, (addr_t*) address, rw);
  }

  inline void
  tm2c_ht_delete_node(tm2c_ht_t tm2c_ht, nodeid_t node_id)
  {
    ssht_log_set_t* log = logs[node_id];
    ssht_log_entry_t* entries = log->log_entries;
    if (log->nb_entries)
      {
	uint32_t j;
	for (j = 0; j < log->nb_entries; j++) 
	  {
	    if (entries[j].address != NULL)
	      {
		ssht_remove_any(entries[j].address, node_id, entries[j].entry);
	      }
	  }
	ssht_log_set_empty(log);
      }
  }

  inline void
  tm2c_ht_print(tm2c_ht_t tm2c_ht)
  {
  }

#endif

#ifdef    __cplusplus
}
#endif
