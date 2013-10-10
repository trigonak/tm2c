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

#include "common.h"
#include "tm2c_mem.h"

    /* ################################################################### *
     * TYPES
     * ################################################################### */

void*
stm_malloc(mem_info_t *stm_mem_info, size_t size)
{
  /* Memory will be freed upon abort */
  mem_block_t *mb; 

  if ((mb = (mem_block_t *) malloc(sizeof (mem_block_t))) == NULL)
    {
      PRINT("malloc @ stm_malloc");
      EXIT(1);
    }
  if ((mb->addr = malloc(size)) == NULL)
    {
      free(mb);
      PRINT("malloc @ stm_malloc");
      EXIT(1);
    }
  mb->next = stm_mem_info->allocated;
  stm_mem_info->allocated = mb;

  return mb->addr;
}

    /*
     * Called by the CURRENT thread to allocate shared memory within a transaction.
     */
void*
stm_shmalloc(mem_info_t *stm_mem_info, size_t size)
{
  /* Memory will be freed upon abort */
  mem_block_t *mb; 

  if ((mb = (mem_block_t *) malloc(sizeof (mem_block_t))) == NULL)
    {
      PRINT("malloc @ stm_shmalloc");
      EXIT(1);
    }
  if ((mb->addr = (void *) sys_shmalloc(size)) == NULL)
    {
      free(mb);
      PRINT("sys_shmalloc @ stm_shmalloc");
      EXIT(1);
    }
  mb->next = stm_mem_info->allocated_shmem;
  stm_mem_info->allocated_shmem = mb;

  return mb->addr;
}

    /*
     * Called by the CURRENT thread to free memory within a transaction.
     */
void
stm_free(mem_info_t *stm_mem_info, void *addr)
{
  /* Memory disposal is delayed until commit */
  mem_block_t *mb; 

  /* Schedule for removal */
  if ((mb = (mem_block_t *) malloc(sizeof (mem_block_t))) == NULL)
    {
      PRINT("malloc @ stm_free");
      EXIT(1);
    }
  mb->addr = addr;
  mb->next = stm_mem_info->freed;
  stm_mem_info->freed = mb;
}

    /*
     * Called by the CURRENT thread to free memory within a transaction.
     */
void
stm_shfree(mem_info_t *stm_mem_info, void* addr)
{
  /* Memory disposal is delayed until commit */
  mem_block_t *mb; 

  /* Schedule for removal */
  if ((mb = (mem_block_t *) malloc(sizeof (mem_block_t))) == NULL) {
    PRINT("malloc @ stm_shfree");
    EXIT(1);
  }
  mb->addr = (void *) addr;
  mb->next = stm_mem_info->freed_shmem;
  stm_mem_info->freed_shmem = mb;
}

    /*
     * Called to create new mem_info
     */
mem_info_t*
mem_info_new()
{
  mem_info_t *mi__;

  if ((mi__ = (mem_info_t *) malloc(sizeof (mem_info_t))) == NULL) {
    PRINT("malloc @ mem_info_new");
    EXIT(1);
  }
  mi__->allocated = mi__->freed = NULL;
  mi__->allocated_shmem = mi__->freed_shmem = NULL;

  return mi__;
}
    
    
    /*
     * Called to free the memory info
     */
inline void
mem_info_free(mem_info_t * mi)
{
  free(mi);
}
    
    /*
     * Called to free the global stm_mem_info variable
     */
inline void
stm_mem_info_free(mem_info_t *stm_mem_info)
{
  free(stm_mem_info);
}


    /*
     * Called upon transaction commit.
     */
void
mem_info_on_commit(mem_info_t *stm_mem_info)
{
  mem_block_t *mb, *next;

  /* Keep memory allocated during transaction */
  if (stm_mem_info->allocated != NULL)
    {
      mb = stm_mem_info->allocated;
      while (mb != NULL)
	{
	  next = mb->next;
	  free(mb);
	  mb = next;
	}
      stm_mem_info->allocated = NULL;
    }

  /* Keep shared memory allocated during transaction */
  if (stm_mem_info->allocated_shmem != NULL)
    {
      mb = stm_mem_info->allocated_shmem;
      while (mb != NULL)
	{
	  next = mb->next;
	  free(mb);
	  mb = next;
	}
      stm_mem_info->allocated_shmem = NULL;
    }

  /* Dispose of memory freed during transaction */
  if (stm_mem_info->freed != NULL)
    {
      mb = stm_mem_info->freed;
      while (mb != NULL)
	{
	  next = mb->next;
	  free(mb->addr);
	  free(mb);
	  mb = next;
	}
      stm_mem_info->freed = NULL;
    }

  /* Dispose of shared memory freed during transaction */
  if (stm_mem_info->freed_shmem != NULL)
    {
      mb = stm_mem_info->freed_shmem;
      while (mb != NULL)
	{
	  next = mb->next;
	  sys_shfree((void*) mb->addr);
	  free(mb);
	  mb = next;
	}
      stm_mem_info->freed_shmem = NULL;
    }
}

    /*
     * Called upon transaction abort.
     */
void
mem_info_on_abort(mem_info_t *stm_mem_info)
{
  mem_block_t *mb, *next;

  /* Dispose of memory allocated during transaction */
  if (stm_mem_info->allocated != NULL)
    {
      mb = stm_mem_info->allocated;
      while (mb != NULL)
	{
	  next = mb->next;
	  free(mb->addr);
	  free(mb);
	  mb = next;
	}
      stm_mem_info->allocated = NULL;
    }

  /* Dispose of shared memory allocated during transaction */
  if (stm_mem_info->allocated_shmem != NULL)
    {
      mb = stm_mem_info->allocated_shmem;
      while (mb != NULL)
	{
	  next = mb->next;
	  sys_shfree((void*) mb->addr);
	  free(mb);
	  mb = next;
	}
      stm_mem_info->allocated_shmem = NULL;
    }

  /* Keep memory freed during transaction */
  if (stm_mem_info->freed != NULL)
    {
      mb = stm_mem_info->freed;
      while (mb != NULL)
	{
	  next = mb->next;
	  free(mb);
	  mb = next;
	}
      stm_mem_info->freed = NULL;
    }

  /* Keep shared memory freed during transaction */
  if (stm_mem_info->freed_shmem != NULL)
    {
      mb = stm_mem_info->freed_shmem;
      while (mb != NULL)
	{
	  next = mb->next;
	  free(mb);
	  mb = next;
	}
      stm_mem_info->freed_shmem = NULL;
    }
}
