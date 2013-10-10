/*
 *   File: ssht.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: interface and structures of ssht 
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

#ifndef _SSHT_H_
#define _SSHT_H_

#include <sys/time.h>
#include <inttypes.h>
#include <limits.h>

#include "common.h"
#include "hash.h"

#include "ssht_log.h"

#if defined(BIT_OPTS)
#include "rw_entry_ssht.h"
#endif	/* BIT_OPTS */

#include <assert.h>
#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
#  include "tm2c_cm.h"
#endif 

#define SIZE_ENTRY 4
#define ADDR_PER_CL 3
#define ENTRY_PER_CL ADDR_PER_CL

#ifdef SCC
#define PADDING_BYTES (64 - (ADDR_PER_CL+1)*4)
#else
#define PADDING_BYTES 0
#endif	/* SCC */

#if defined(BIT_OPTS)
#define MAX_READERS 64
#else
#define MAX_READERS TM2C_MAX_PROCS
#endif	/* BIT_OPTS */

#define SSHT_NO_WRITER 0xFF
#if defined(SCC)
#  define SSHT_ENTRY_FREE 0x000000FF
#else
#  define SSHT_ENTRY_FREE 0x00000000000000FF
#endif

#define FALSE 0
#define TRUE 1

#define NUM_OF_BUCKETS   64
#define NUM_OF_BUCKETS_2 63
#define NUM_BUCKETS NUM_OF_BUCKETS

typedef uintptr_t addr_t;

#if !defined(BIT_OPTS)
typedef struct ALIGNED(CACHE_LINE_SIZE) ssht_rw_entry 
{
  uint8_t nr;
  uint8_t reader[MAX_READERS];
  uint8_t writer;
} ssht_rw_entry_t;
#endif	/* !BIT_OPTS */

typedef struct ALIGNED(CACHE_LINE_SIZE) bucket
{         
  addr_t addr[ADDR_PER_CL];	             
  struct bucket* next;			     
  uint8_t padding[PADDING_BYTES];
  ssht_rw_entry_t entry[ENTRY_PER_CL];
} bucket_t;


#if defined(SSHT_DBG_UTILIZATION)
extern uint32_t ssht_dbg_bu_expansions;
extern uint32_t ssht_dbg_usages;
extern uint32_t ssht_dbg_bu_usages[NUM_BUCKETS];
extern uint32_t ssht_dbg_bu_usages_w[NUM_BUCKETS];
extern uint32_t ssht_dbg_bu_usages_r[NUM_BUCKETS];
#endif	/* SSHT_DBG_UTILIZATION */

typedef bucket_t* ssht_hashtable_t;

extern ssht_hashtable_t ssht_new();
extern void ssht_free(ssht_hashtable_t*);

extern void bucket_print(bucket_t* bu);
extern void ssht_stats_print(ssht_hashtable_t ht, uint32_t details);

extern TM2C_CONFLICT_T bucket_insert_r(bucket_t* bu, ssht_log_set_t* log, uint32_t id, uintptr_t addr); 
extern TM2C_CONFLICT_T bucket_insert_w(bucket_t* bu, ssht_log_set_t* log, uint32_t id, uintptr_t addr);

INLINED bucket_t* 
ssht_bucket_new() 
{
#if defined(SSHT_DBG_UTILIZATION)
  ssht_dbg_bu_expansions++;
#endif	/* SSHT_DBG_UTILIZATION */
  bucket_t* bu = (bucket_t *) calloc(1, sizeof(bucket_t));
  assert(bu != NULL);

  uint32_t i;
  for (i = 0; i < ENTRY_PER_CL; i++) 
    {
      bu->entry[i].writer = SSHT_NO_WRITER;
    }

  return bu;
}


#define ssht_rw_entry_has_readers(entry) (entry)->nr

INLINED TM2C_CONFLICT_T 
ssht_insert(ssht_hashtable_t ht, uint32_t bu, ssht_log_set_t* log, uint32_t id, uintptr_t addr, RW rw) 
{
#if defined(SSHT_DBG_UTILIZATION)
  ssht_dbg_usages++;
  ssht_dbg_bu_usages[bu]++;
#endif	/* SSHT_DBG_UTILIZATION */
  TM2C_CONFLICT_T ct;
  if (rw == READ) 
    {
#if defined(SSHT_DBG_UTILIZATION)
      ssht_dbg_bu_usages_r[bu]++;
#endif	/* SSHT_DBG_UTILIZATION */
      ct = bucket_insert_r(ht + bu, log, id, addr);
    }
  else 
    {
#if defined(SSHT_DBG_UTILIZATION)
      ssht_dbg_bu_usages_w[bu]++;
#endif	/* SSHT_DBG_UTILIZATION */
      ct = bucket_insert_w(ht + bu, log, id, addr);
    }

  return ct;
}


INLINED uint32_t 
ssht_bucket_remove_index(addr_t *addr, uint32_t id, ssht_rw_entry_t *entry) 
{
#if defined(BIT_OPTS)
  if (entry->writer == id) 
    {
      entry->writer = SSHT_NO_WRITER;
    }
  else 
    {
      rw_entry_ssht_unset(entry, id);
    }
    
  if (rw_entry_ssht_is_empty(entry))
    {
      *addr = 0;
    }

#else
  if (entry->writer == id) 
    {
      entry->writer = SSHT_NO_WRITER;
    }
  else 
    {
      entry->reader[id] = 0;
      entry->nr--;
    }
    
  if (entry->nr == 0 && entry->writer == SSHT_NO_WRITER) 
    {
      *addr = 0;
    }

#endif	/* BIT_OPTS */
  return TRUE;
}

INLINED uint32_t 
ssht_remove_any(addr_t *addr, uint32_t id, ssht_rw_entry_t *entry) 
{
  return ssht_bucket_remove_index(addr, id, entry);
}

INLINED void 
ssht_remove(ssht_log_set_t* log, uint32_t id, addr_t* addr, RW rw) 
{
  ssht_log_entry_t *entries = log->log_entries;
  uint32_t j;
  for (j = 0; j < log->nb_entries; j++) 
    {
      if (entries[j].address != NULL && *entries[j].address == (uintptr_t) addr)
	{
	  ssht_rw_entry_t* entry = entries[j].entry;
	  if (rw == WRITE) 
	    {
	      if (entry->writer == id)
		{
		  entry->writer = SSHT_NO_WRITER;
		}
	    }

#if defined(BIT_OPTS)
	  else 
	    {
	      rw_entry_ssht_unset(entry, id);
	    }
    
#else
	  else 
	    {
	      entry->reader[id] = 0;
	      entry->nr--;
	    }
    
	  /* WHY not?? */
	  /* if (entry->nr == 0 && entry->writer == SSHT_NO_WRITER)  */
	  /*   { */
	  /*     *addr = 0; */
	  /*   } */
#endif	/* BIT_OPTS */

	  /* entries[j].address = NULL; */
	  int last = --log->nb_entries;
	  entries[j].address = entries[last].address;
	  entries[j].entry = entries[last].entry;
	  break;
	}
    }
}


INLINED void 
ht_print(bucket_t * ht) 
{
  unsigned int i;
  for (i = 0; i < NUM_BUCKETS; i++) 
    {
      printf("[%02d]: ", i);
      bucket_print(ht + i);
    }
}

#endif /* _SSHT_H_ */
