/*
 *   File: ssht.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: Super-Simple Hash Table a fixed-bucket hash table
 *                which include the contention manager calls
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

#include "ssht.h"

#include <malloc.h>

#if defined(SSHT_DBG_UTILIZATION)
uint32_t ssht_dbg_usages = 0;
uint32_t ssht_dbg_bu_usages[NUM_BUCKETS] = {0};
uint32_t ssht_dbg_bu_expansions = 0;
uint32_t ssht_dbg_bu_usages_w[NUM_BUCKETS] = {0};
uint32_t ssht_dbg_bu_usages_r[NUM_BUCKETS] = {0};
#endif	/* SSHT_DBG_UTILIZATION */

ssht_hashtable_t 
ssht_new() 
{
  ssht_hashtable_t hashtable;
  hashtable = (ssht_hashtable_t) memalign(CACHE_LINE_SIZE, NUM_BUCKETS * sizeof(bucket_t));

  assert(hashtable != NULL);
  assert((intptr_t) hashtable % CACHE_LINE_SIZE == 0);
  assert(sizeof(bucket_t) % CACHE_LINE_SIZE == 0);

  uint64_t* ht_uint64 = (uint64_t*) hashtable;
  uint32_t i;

  for (i = 0; i < (NUM_BUCKETS * sizeof(bucket_t)) / sizeof(uint64_t); i++)
    {
      ht_uint64[i] = 0;
    }

  for (i = 0; i < NUM_BUCKETS; i++) 
    {
      uint32_t j;
      for (j = 0; j < ENTRY_PER_CL; j++) 
	{
	  hashtable[i].entry[j].writer = SSHT_NO_WRITER;
	}
    }

  return hashtable;
}

void
ssht_free(ssht_hashtable_t* ht)
{
  free(ht);
}

void 
bucket_print(bucket_t* bu) 
{
#if !defined(BIT_OPTS)
  bucket_t* btmp = bu;
  do 
    {
      uint32_t j;
      for (j = 0; j < ADDR_PER_CL; j++) 
	{
	  printf("%p:%2d/%d|", (void*)btmp->addr[j], btmp->entry[j].nr, btmp->entry[j].writer);
	}
      btmp = btmp->next;
      printf("|");
    } 
  while (btmp != NULL);
  printf("\n");
#endif	/* BIT_OPTS */
}


TM2C_CONFLICT_T 
bucket_insert_r(bucket_t* bu, ssht_log_set_t* log, uint32_t id, uintptr_t addr) 
{
  uint32_t i;
  bucket_t* btmp = bu;
  do 
    {
      for (i = 0; i < ADDR_PER_CL; i++) 
	{
	  if (btmp->addr[i] == addr) 
	    {
	      ssht_rw_entry_t* e = btmp->entry + i;
	      if (e->writer != SSHT_NO_WRITER) 
		{
#if !defined(NOCM) && !defined(BACKOFF_RETRY) 			/* if any other CM (greedy, wholly, faircm) */
		  if (!contention_manager_raw_waw(id, (uint16_t) e->writer, READ_AFTER_WRITE)) 
		    {
		      return READ_AFTER_WRITE;
		    }
#else
		  return READ_AFTER_WRITE;
#endif	/* NOCM */
		}

	      btmp->addr[i] = addr;
#if defined(BIT_OPTS)
	      rw_entry_ssht_set(e, id);
	      ssht_log_set_insert(log, btmp->addr + i, e);
#else
	      if (!e->reader[id])
	      	{
		  e->nr++;
		  e->reader[id] = 1;
		  ssht_log_set_insert(log, btmp->addr + i, e);
		}
#endif	/* BIT_OPTS */

	      return NO_CONFLICT;
	    }
	}

      btmp = btmp->next;
    } 
  while (btmp != NULL);

  btmp = bu;

  do 
    {
      for (i = 0; i < ADDR_PER_CL; i++) 
	{
	  if (btmp->addr[i] == 0) 
	    {
	      ssht_rw_entry_t* e = btmp->entry + i;
	      btmp->addr[i] = addr;
#if defined(BIT_OPTS)
	      rw_entry_ssht_set(e, id);
#else
	      e->nr++;
	      e->reader[id] = 1;
#endif	/* BIT_OPTS */
	      ssht_log_set_insert(log, btmp->addr + i, e);
	      return NO_CONFLICT;
	    }
	}

      if (btmp->next == NULL) 
	{
	  btmp->next = ssht_bucket_new();
	}

      btmp = btmp->next;
    } 
  while (1);
}  

TM2C_CONFLICT_T 
bucket_insert_w(bucket_t* bu, ssht_log_set_t* log, uint32_t id, uintptr_t addr) 
{
  uint32_t i;
  bucket_t* btmp = bu;
  do 
    {
      for (i = 0; i < ADDR_PER_CL; i++) 
	{
	  if (btmp->addr[i] == addr)                               /* there is an entry for this addr */
	    {
	      ssht_rw_entry_t* e = btmp->entry + i;
	      if (e->writer != SSHT_NO_WRITER && e->writer != id) /* there is a writer for this entry */
		{
#if !defined(NOCM) && !defined(BACKOFF_RETRY)                    /* any other CM (greedy, wholly, faircm) */
		  if (contention_manager_raw_waw(id, e->writer, WRITE_AFTER_WRITE)) 
		    {
		      btmp->addr[i] = addr;
		      e->writer = id;
		      ssht_log_set_insert(log, btmp->addr + i, e);
		      return NO_CONFLICT;
		    }
		  else 
		    {
		      return WRITE_AFTER_WRITE;
		    }
#else   /* NOCM */
		  return WRITE_AFTER_WRITE;
#endif	/* NOCM */
		}
#if defined(BIT_OPTS)
	      else if (rw_entry_ssht_has_readers(e) && !rw_entry_ssht_is_unique_reader(e, id))
#else
	      else if (e->nr > 1 || (e->nr == 1 && e->reader[id] == 0))
#endif	/* BIT_OPTS */
		{
#if !defined(NOCM) && !defined(BACKOFF_RETRY) 			/* if any other CM (greedy, wholly, faircm) */
#  if defined(BIT_OPTS)
		  uint8_t readers[MAX_READERS];
		  rw_entry_ssht_fetch_readers(e, readers);
#  else
		  uint8_t* readers = e->reader;
#  endif

		  if (contention_manager_war(id, readers, WRITE_AFTER_READ))
		    {
		      btmp->addr[i] = addr;
		      e->writer = id;
		      ssht_log_set_insert(log, btmp->addr + i, e);
		      return NO_CONFLICT;
		    }
		  else
		    {
		      return WRITE_AFTER_READ;
		    }
#else
		  return WRITE_AFTER_READ;
#endif	/* NOCM */
		}
	      else		/* 1 reader and this reader is node id */
		{
		  btmp->addr[i] = addr;
		  e->writer = id;
		  ssht_log_set_insert(log, btmp->addr + i, e);
		  return NO_CONFLICT;
		}
	    }
	}
    
      btmp = btmp->next;
    }
  while (btmp != NULL);

  btmp = bu;
  do
    {
      for (i = 0; i < ADDR_PER_CL; i++)
	{
	  if (btmp->addr[i] == 0) 
	    {
	      ssht_rw_entry_t* e = btmp->entry + i;
	      e->writer = id;
	      btmp->addr[i] = addr;
	      ssht_log_set_insert(log, btmp->addr + i, e);
	      return NO_CONFLICT;
	    }
	}

      if (btmp->next == NULL)
	{
	  btmp->next = ssht_bucket_new();
	}

      btmp = btmp->next;
    } 
  while (1);
}

void 
ssht_stats_print(ssht_hashtable_t ht, uint32_t details)
{
#if defined(SSHT_DBG_UTILIZATION)
  printf("SSHT stats: core %02d  /  ", NODE_ID());
  printf("total insertions: %-14u  /  num expansions  : %u\n", ssht_dbg_usages, ssht_dbg_bu_expansions);
  if (details)
    {
      printf(" per bucket:\n");
      uint32_t b;
      for (b = 0; b < NUM_BUCKETS; b++)
	{
	  double usage_perc = 100 * ((double) ssht_dbg_bu_usages[b] / ssht_dbg_usages);
	  printf("# %3u :: usages: %8u  = %5.2f%%  (reads: %8u | writes: %8u)\n",
		 b, ssht_dbg_bu_usages[b], usage_perc, ssht_dbg_bu_usages_r[b], ssht_dbg_bu_usages_w[b]);
	}
    }
#endif	/* SSHT_DBG_UTILIZATION */
}
