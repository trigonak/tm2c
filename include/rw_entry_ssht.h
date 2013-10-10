/*
 *   File: rw_entry_ssht.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description:  data structures to be used for ssht (up to 64 readers)
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

#ifndef RW_ENTRY_SSHT_H
#define	RW_ENTRY_SSHT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include "common.h"
#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
#include "tm2c_cm.h"
#endif

  /*__________________________________________________________________________________________________
   * RW ENTRY                                                                                         |
   * _________________________________________________________________________________________________|
   */

  /* #define NO_WRITERI 0x10000000 */
  /* #define NO_WRITERLL 0x1000000000000000ULL */
  /* #define NO_WRITER 0x1000 */

#define NO_READERS 0x0
#define NO_WRITERC 0xFF

  typedef struct ssht_rw_entry 
  {
    uint64_t readers;
    uint8_t writer;
  } ssht_rw_entry_t;

  /*___________________________________________________________________________________________________
   * Functions
   *___________________________________________________________________________________________________
   */

  INLINED BOOLEAN rw_entry_ssht_is_member(ssht_rw_entry_t* rwe, nodeid_t node_id);

  INLINED void rw_entry_ssht_set(ssht_rw_entry_t* rwe, nodeid_t node_id);

  INLINED void rw_entry_ssht_unset(ssht_rw_entry_t* rwe, nodeid_t node_id);

  INLINED void rw_entry_ssht_clear(ssht_rw_entry_t* rwe);

  INLINED BOOLEAN rw_entry_ssht_is_empty(ssht_rw_entry_t* rwe);

  INLINED BOOLEAN rw_entry_ssht_has_readers(ssht_rw_entry_t* rwe);

  INLINED BOOLEAN rw_entry_ssht_is_unique_reader(ssht_rw_entry_t* rwe, nodeid_t node_id);

  INLINED void rw_entry_ssht_set_writer(ssht_rw_entry_t* rwe, nodeid_t node_id);

  INLINED void rw_entry_ssht_unset_writer(ssht_rw_entry_t* rwe);

  INLINED BOOLEAN rw_entry_ssht_has_writer(ssht_rw_entry_t* rwe);

  INLINED BOOLEAN rw_entry_ssht_is_writer(ssht_rw_entry_t* rwe, nodeid_t node_id);

  INLINED ssht_rw_entry_t* rw_entry_ssht_new();

  INLINED TM2C_CONFLICT_T rw_entry_ssht_is_conflicting(ssht_rw_entry_t* rw_entry, nodeid_t node_id, RW rw);

  INLINED void rw_entry_ssht_print_readers(ssht_rw_entry_t* rwe);

  /*___________________________________________________________________________________________________
   * Implementation
   *___________________________________________________________________________________________________
   */

  INLINED BOOLEAN
  rw_entry_ssht_is_member(ssht_rw_entry_t* rwe, nodeid_t node_id)
  {
    return (BOOLEAN) ((rwe->readers >> node_id) & 0x1);
  }

  INLINED void
  rw_entry_ssht_set(ssht_rw_entry_t* rwe, nodeid_t node_id)
  {
    rwe->readers |= (1LL << node_id);
  }

  INLINED void
  rw_entry_ssht_unset(ssht_rw_entry_t* rwe, nodeid_t node_id)
  {
    rwe->readers &= ~(1LL << node_id);
  }

  INLINED void
  rw_entry_ssht_clear(ssht_rw_entry_t* rwe)
  {
    rwe->readers = NO_READERS;
    rwe->writer = NO_WRITERC;
  }

  INLINED BOOLEAN
  rw_entry_ssht_is_empty(ssht_rw_entry_t* rwe)
  {
    return (BOOLEAN) (rwe->readers == NO_READERS && rwe->writer == NO_WRITERC);
  }

  INLINED BOOLEAN
  rw_entry_ssht_has_readers(ssht_rw_entry_t* rwe)
  {
    return (BOOLEAN) (rwe->readers != NO_READERS);
  }


  INLINED BOOLEAN
  rw_entry_ssht_is_unique_reader(ssht_rw_entry_t* rwe, nodeid_t node_id)
  {
    ssht_rw_entry_t tmp;
    tmp.readers = NO_READERS;
    rw_entry_ssht_set(&tmp, node_id);
    return (BOOLEAN) (tmp.readers == rwe->readers);
  }

  INLINED void
  rw_entry_ssht_fetch_readers(ssht_rw_entry_t* rwe, uint8_t* readers) 
  {
    uint64_t convert = rwe->readers;

    int i;
    for (i = 0; i < NUM_UES; i++) 
      {
	if (convert & 0x01) 
	  {
	    readers[i] = 1;
	  }
	else 
	  {
	    readers[i] = 0;
	  }
	convert >>= 1;
      }
  }

  INLINED void
  rw_entry_ssht_set_writer(ssht_rw_entry_t* rwe, nodeid_t node_id)
  {
    rwe->writer = node_id;
  }

  INLINED void
  rw_entry_ssht_unset_writer(ssht_rw_entry_t* rwe)
  {
    rwe->writer = NO_WRITERC;
  }

  INLINED BOOLEAN
  rw_entry_ssht_has_writer(ssht_rw_entry_t* rwe)
  {
    return (BOOLEAN) (rwe->writer != NO_WRITERC);
  }

  INLINED BOOLEAN
  rw_entry_ssht_is_writer(ssht_rw_entry_t* rwe, nodeid_t node_id)
  {
    return (BOOLEAN) (rwe->writer == node_id);
  }

  INLINED ssht_rw_entry_t*
  rw_entry_ssht_new()
  {
    ssht_rw_entry_t* r = (ssht_rw_entry_t*) malloc(sizeof(ssht_rw_entry_t));
    r->readers = NO_READERS;
    r->writer = NO_WRITERC;
    return r;
  }

  INLINED void 
  rw_entry_ssht_print_readers(ssht_rw_entry_t* rwe) 
  {
    uint64_t convert = rwe->readers;
    int i;
    for (i = 0; i < MAX_READERS; i++) 
      {
	if (convert & 0x01) 
	  {
	    PRINTS("%d -> ", i);
	  }
	convert >>= 1;
      }

    PRINTS("NULL\n");
    FLUSH;
  }

#ifdef	__cplusplus
}
#endif

#endif	/* RW_ENTRY_SSHT_H */
