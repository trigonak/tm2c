/*
 *   File: ssht_log.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: log of accesses to ssht
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

#ifndef SSHT_LOG_H
#define	SSHT_LOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include "ssht.h"

#ifndef INLINED
#if __GNUC__ && !__GNUC_STDC_INLINE__
#define INLINED static inline __attribute__((always_inline))
#else
#define INLINED inline
#endif
#endif

#define SSHT_LOG_SET_SIZE 8

  typedef struct ssht_log_entry 
  {
    uintptr_t* address;    //* to the addr index for this entry
    struct ssht_rw_entry* entry; //* to the specific entry
  } ssht_log_entry_t;

  typedef struct ssht_log_set 
  {
    ssht_log_entry_t *log_entries;
    uint32_t nb_entries;
    uint32_t size;
  } ssht_log_set_t;

  INLINED
  ssht_log_set_t* ssht_log_set_new() 
  {
    ssht_log_set_t *ssht_log_set;

    if ((ssht_log_set = (ssht_log_set_t *) malloc(sizeof (ssht_log_set_t))) == NULL) 
      {
	perror("Could not initialize the ssht_log set");
	return NULL;
      }

    if ((ssht_log_set->log_entries = (ssht_log_entry_t *) malloc(SSHT_LOG_SET_SIZE * sizeof (ssht_log_entry_t))) == NULL) 
      {
	free(ssht_log_set);
	perror("Could not initialize the ssht_log set");
	return NULL;
      }

    ssht_log_set->nb_entries = 0;
    ssht_log_set->size = SSHT_LOG_SET_SIZE;

    return ssht_log_set;
  }


  INLINED void ssht_log_set_free(ssht_log_set_t *ssht_log_set) {
    free(ssht_log_set->log_entries);
    free(ssht_log_set);
  }


  INLINED ssht_log_set_t * ssht_log_set_empty(ssht_log_set_t *ssht_log_set) {
    ssht_log_set->nb_entries = 0;
    return ssht_log_set;
  }

  INLINED ssht_log_entry_t * ssht_log_set_entry(ssht_log_set_t *ssht_log_set) {
    if (ssht_log_set->nb_entries == ssht_log_set->size) {
/*       PRINT("LOCK set max sized (%d)", ssht_log_set->size); */
      unsigned int new_size = 2 * ssht_log_set->size;
      ssht_log_entry_t *temp;
      if ((temp = (ssht_log_entry_t *) realloc(ssht_log_set->log_entries, new_size * sizeof (ssht_log_entry_t))) == NULL) {
	ssht_log_set_free(ssht_log_set);
	printf("Could not resize the ssht_log set\n");
	return NULL;
      }

      ssht_log_set->log_entries = temp;
      ssht_log_set->size = new_size;
    }

    return &ssht_log_set->log_entries[ssht_log_set->nb_entries++];
  }

  INLINED void ssht_log_set_insert(ssht_log_set_t *ssht_log_set, uintptr_t *addr, struct ssht_rw_entry *entry) {
    ssht_log_entry_t *we = ssht_log_set_entry(ssht_log_set);
    we->address = addr;
    we->entry = entry;
  }

#ifdef	__cplusplus
}
#endif

#endif	/* LOG_H */

