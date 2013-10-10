/*
 *   File: tm2c_cm.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: contention managemement
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

#include "tm2c_cm.h"

#include "tm2c_dsl_ht.h"
extern tm2c_ht_t tm2c_ht;

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* any CM: wholly, greedy, faircm */

#if defined(PGAS)
#include "tm2c_log.h"
extern tm2c_write_set_pgas_t** PGAS_write_sets;
#endif

inline BOOLEAN 
contention_manager_raw_waw(nodeid_t attacker, uint16_t defender, TM2C_CONFLICT_T conflict) 
{
  if (cm_metadata_core[attacker].timestamp < cm_metadata_core[defender].timestamp ||
      (cm_metadata_core[attacker].timestamp == cm_metadata_core[defender].timestamp && attacker < defender)) 
    {
      //new TX - attacker won
#if defined(PGAS)
      if (abort_node_is_persisting((uint32_t) defender, conflict))
	{
	  write_set_pgas_persist(PGAS_write_sets[defender]);
	}
      write_set_pgas_empty(PGAS_write_sets[defender]);
#else
      abort_node((uint32_t) defender, conflict);
      tm2c_ht_delete_node(tm2c_ht, defender);
#endif	/* PGAS */
      return TRUE;
    } 
  else 				/* existing TX won */
    { 
      return FALSE;
    }

  return FALSE;
}

inline BOOLEAN 
contention_manager_war(nodeid_t attacker, uint8_t* defenders, TM2C_CONFLICT_T conflict)
{
  nodeid_t defender;
  for (defender = 0; defender < NUM_UES; defender++)
    {
      if (defenders[defender])
	{
	  if (cm_metadata_core[attacker].timestamp > cm_metadata_core[defender].timestamp ||
	      (cm_metadata_core[attacker].timestamp == cm_metadata_core[defender].timestamp && defender < attacker))
	    {
	      return FALSE;
	    }
	}
    }
  //attacker won all readers
  for (defender = 0; defender < NUM_UES; defender++)
    {
      if (defenders[defender] && defender != attacker)
	{
#if defined(PGAS)
	  if (abort_node_is_persisting((uint32_t) defender, conflict))
	    {
	      write_set_pgas_persist(PGAS_write_sets[defender]);
	    }
	  write_set_pgas_empty(PGAS_write_sets[defender]);
#else
      abort_node((unsigned int) defender, conflict);
      tm2c_ht_delete_node(tm2c_ht, defender);
#endif	/* PGAS */
	}
    }

  return TRUE;
}

void
contention_manager_pri_print() 
{
  uint32_t reps = TOTAL_NODES();
  uint32_t *found = (uint32_t *) calloc(reps, sizeof(uint32_t));
  assert(found != NULL);

  printf("\t");

  while (reps--) 
    {
      uint64_t max = 100000000, max_index = max;
      uint32_t i;
      for (i = 0; i < TOTAL_NODES(); i++) 
	{
	  if (!found[i]) {
	    if (cm_metadata_core[i].timestamp < max ||
		(cm_metadata_core[i].timestamp == max && i < max_index)) 
	      {
		max = cm_metadata_core[i].timestamp;
		max_index = i;
	      }
	  }
	}
      found[max_index] = 1;
      if (max > 0) 
	{
	  printf("%02llu [%llu] > ", (long long unsigned int) max_index, (long long unsigned int) max);
	}
    }
  printf("none\n");
  FLUSH;
}

inline BOOLEAN 
contention_manager(nodeid_t attacker, unsigned short *defenders, TM2C_CONFLICT_T conflict) {
  switch (conflict)
    {
    case READ_AFTER_WRITE:
    case WRITE_AFTER_WRITE:
      if (cm_metadata_core[attacker].timestamp < cm_metadata_core[*defenders].timestamp ||
	  (cm_metadata_core[attacker].timestamp == cm_metadata_core[*defenders].timestamp && attacker < *defenders))
	{
	  //new TX - attacker won
#if defined(PGAS)
	  if (abort_node_is_persisting((uint32_t) *defenders, conflict))
	    {
	      write_set_pgas_persist(PGAS_write_sets[*defenders]);
	    }
	  write_set_pgas_empty(PGAS_write_sets[*defenders]);
#else
	  abort_node((unsigned int) *defenders, conflict);
	  tm2c_ht_delete_node(tm2c_ht, *defenders);
#endif	/* PGAS */
	//it was running and the current node aborted it
	  PRINTD("[force abort] %d (%d) for %d (%d)",
		 *defenders, cm_metadata_core[*defenders].num_txs,
		 attacker,
		 cm_metadata_core[attacker].num_txs);
	  return TRUE;
	} 
      else
	{ //existing TX won
	  PRINTD("[norml abort] %d (%d) for %d (%d)",
		 attacker,
		 cm_metadata_core[attacker].num_txs,
		 *defenders, cm_metadata_core[*defenders].num_txs);
	  return FALSE;
	}
    case WRITE_AFTER_READ:
      {
	int i;
	for (i = 0; i < NUM_UES; i++)
	  {
	    if (defenders[i])
	      {
		if (cm_metadata_core[attacker].timestamp > cm_metadata_core[i].timestamp ||
		    (cm_metadata_core[attacker].timestamp == cm_metadata_core[i].timestamp && i < attacker))
		  {
		    PRINTD("[norml abort] %d (%d) for %d (%d)",
			   attacker,
			   cm_metadata_core[attacker].num_txs,
			   i, cm_metadata_core[i].num_txs);
                            
		    return FALSE;
		  }
	      }
	  }
	for (i = 0; i < NUM_UES; i++)
	  {
	    if (defenders[i])
	      {
#if defined(PGAS)
		if (abort_node_is_persisting((uint32_t) i, conflict))
		  {
		    write_set_pgas_persist(PGAS_write_sets[i]);
		  }
		write_set_pgas_empty(PGAS_write_sets[i]);
#else
		abort_node((unsigned int) i, conflict);
		tm2c_ht_delete_node(tm2c_ht, i);
#endif	/* PGAS */
	      }
	  }

	PRINTD("[force abort] READERS for %d (%d)",
	       attacker,
	       cm_metadata_core[attacker].num_txs);
	return TRUE;
      }
    default:
      {
	
      }
    }
  //to avoid non ret warning
  return FALSE;
}

#endif	/* NOCM */
