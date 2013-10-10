/*
 * Adapted to TM2C by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>  
 *
 * File:
 *   mod_mem.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Module for dynamic memory management.
 *
 * Copyright (c) 2007-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* 
 * File:   mem.h
 * Author: trigonak
 *
 * Created on May 23, 2011, 10:55 AM
 */

#ifndef _TM2C_MEM_H
#define	_TM2C_MEM_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "common.h"

    /* ################################################################### *
     * TYPES
     * ################################################################### */

    typedef struct mem_block { /* Block of allocated memory */
        void *addr; /* Address of memory */
        struct mem_block *next; /* Next block */
    } mem_block_t;

    typedef struct mem_info { /* Memory descriptor */
        mem_block_t *allocated; /* Memory allocated by this transaction (freed upon abort) */
        mem_block_t *allocated_shmem; /* Shared Memory allocated by this transaction (freed upon abort) */
        mem_block_t *freed; /* Memory freed by this transaction (freed upon commit) */
        mem_block_t *freed_shmem; /* Shared Memory freed by this transaction (freed upon commit) */
    } mem_info_t;

    /* ################################################################### *
     * FUNCTIONS
     * ################################################################### */

    /*
     * Called by the CURRENT thread to allocate memory within a transaction.
     */
  void *stm_malloc(mem_info_t *stm_mem_info, size_t size);

    /*
     * Called by the CURRENT thread to allocate shared memory within a transaction.
     */
  void *stm_shmalloc(mem_info_t *stm_mem_info, size_t size);

    /*
     * Called by the CURRENT thread to free memory within a transaction.
     */
  void stm_free(mem_info_t *stm_mem_info, void *addr);

    /*
     * Called by the CURRENT thread to free memory within a transaction.
     */
  void stm_shfree(mem_info_t *stm_mem_info, void* addr);

    /*
     * Called to create new mem_info
     */
  mem_info_t * mem_info_new();
    
    
    /*
     * Called to free the memory info
     */
  void mem_info_free(mem_info_t * mi);
    
    /*
     * Called to free the global stm_mem_info variable
     */
  void stm_mem_info_free(mem_info_t *stm_mem_info);

    /*
     * Called upon transaction commit.
     */
  void mem_info_on_commit(mem_info_t *stm_mem_info);

    /*
     * Called upon transaction abort.
     */
  void mem_info_on_abort(mem_info_t *stm_mem_info);

#ifdef	__cplusplus
}
#endif

#endif	/* _TM2C_MEM_H */

