/*
 *   File: tm2c_dsl_ht.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description:  Data structures and operations to be used for DS locking.
 *                 The code is used only by the server (Distributed Lock).
 *                 Hence, all addresses are of type tm_intern_addr_t
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

#ifndef TM2C_HT_H
#define TM2C_HT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <assert.h>
#include "hash.h"
#include "common.h"

/*
 * This section defines the interface the hash table needs to provide.
 * In the section after this one, we provide the implementation for different
 * types of hashtables.
 */

/*
 * tm2c_ht_t denotes the type of the hashtable. It should be customized
 * to each type of hashtable we use.
 * USE_HASHTABLE_SSHT:     Super-Simple-HT (counting readers)
 */
    
#if USE_HASHTABLE_SSHT

#  include "ssht.h"
#  include "ssht_log.h"
  typedef ssht_hashtable_t tm2c_ht_t;

#else
#  error "** No type of hashtable implementation given."
#endif

/*
 * create, initialize and return a tm2c_ht
 */
extern tm2c_ht_t tm2c_ht_new();

/*
 * free the ht
 */
extern void tm2c_ht_free(tm2c_ht_t ht);

/*
 * insert a reader of writer for the address into the hashatable. The hashtable is the constract that keeps
 * all the metadata for the addresses that the node is responsible.
 *
 * Returns: type of TM conflict caused by inserting given values
 */
extern TM2C_CONFLICT_T tm2c_ht_insert(tm2c_ht_t tm2c_ht, nodeid_t nodeId,
                                tm_intern_addr_t address, RW rw);

/*
 * delete a reader of writer for the address from the hashatable.
 */
extern void tm2c_ht_delete(tm2c_ht_t tm2c_ht, nodeid_t nodeId,
                                tm_intern_addr_t address, RW rw);

extern void tm2c_ht_delete_node(tm2c_ht_t tm2c_ht, nodeid_t nodeId);

/*
 * traverse and print the hastable contents
 */
extern void tm2c_ht_print(tm2c_ht_t tm2c_ht);

#ifdef    __cplusplus
}
#endif

#endif    /* TM2C_HT_H */
