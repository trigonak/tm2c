/*
 *   File: pgas_app.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: PGAS memory for the application processes
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
#include "pgas_app.h"

nodeid_t pgas_app_my_resp_node;
nodeid_t pgas_app_my_resp_node_real;
size_t pgas_dsl_size_node;

static void* pgas_app_mem_mine;
static size_t pgas_mem_size;
static size_t alloc_next;
static size_t* pgas_allocs;
static nodeid_t pgas_alloc_rr_next = 0;

#if defined(SCC)		
void* pgas_app_mem;
void* pgas_free_list[256] = {0};
uint8_t pgas_free_cur = 0;
uint8_t pgas_free_num = 0;
/* to avoid void ptr arithmetic warning */
#  define PTR_ADD(ptr, plus) ((void*)  ((char*) (ptr) + (plus)))
#  define PTR_SUB(ptr, plus) ((size_t) ((char*) (ptr) - (plus)))
#else  /* !SCC ---------------------------------------------------------------*/
static void* pgas_app_mem;
static void* pgas_free_list[256] = {0};
static uint8_t pgas_free_cur = 0;
static uint8_t pgas_free_num = 0;
#  define PTR_ADD(ptr, plus) ((ptr) + (plus))
#  define PTR_SUB(ptr, plus) ((ptr) - (plus))
#endif	/* SCC */

void 
pgas_app_init()
{
  pgas_dsl_size_node = PGAS_DSL_SIZE_NODE;
  pgas_mem_size = NUM_DSL_NODES * PGAS_DSL_SIZE_NODE;
  /* do not actually need the memory, just the addr space */
  pgas_app_mem = (void*) 0;	

  alloc_next = 64;

  pgas_allocs = (size_t*) calloc(NUM_DSL_NODES, sizeof(size_t));
  assert(pgas_allocs != NULL);

  int32_t n;
  for (n = 0; n < NUM_DSL_NODES; n++)
    {
      pgas_allocs[n] = (n * PGAS_DSL_SIZE_NODE) + 64;
    }

  assert(NODE_ID() > 0);
  nodeid_t real_id = 0;
  for (n = NODE_ID() - 1; n >= 0; n--)
    {
      if (is_dsl_core(n))
	{
	  pgas_app_my_resp_node = dsl_id_seq(n);
	  pgas_app_my_resp_node_real = n;
	  real_id = n;
	  break;
	}
    }

  /* find how many other APP nodes will be using the same DSL node */
  uint32_t num_app_sharing = 0, my_seq_sharing = 0;
  for (n = real_id + 1; n < TOTAL_NODES(); n++)
    {
      if (is_app_core(n))
	{
	  if (n == NODE_ID())
	    {
	      my_seq_sharing = num_app_sharing;
	    }
	  num_app_sharing++;
	}
      else
	{
	  break;
	}
    }

  size_t my_offset = (pgas_dsl_size_node / num_app_sharing) * my_seq_sharing;
  size_t node_offset = pgas_app_my_resp_node * pgas_dsl_size_node;
  pgas_app_mem_mine = (void*)((uintptr_t) pgas_app_mem + node_offset + my_offset);
  PRINTD(" **  Sending to %3d (realid: %02d) :: off: %10lu :: shared by %2d (my seq %d)", 
	pgas_app_my_resp_node, real_id, (UL) pgas_app_addr_offs(pgas_app_mem_mine), 
	 num_app_sharing, my_seq_sharing);
}

void
pgas_app_term()
{
  free(pgas_allocs);
}

void*
pgas_app_alloc(size_t size)
{
  void* ret;
  if (pgas_free_num > 2)
    {
      uint8_t spot = pgas_free_cur - pgas_free_num;
      ret = pgas_free_list[spot];
      /* PRINT("spot %u", pgas_free_cur - pgas_free_num); */
      pgas_free_num--;
    }
  else
    {
      ret = PTR_ADD(pgas_app_mem_mine, alloc_next);
      alloc_next += size;
    }

  /* PRINT("[lib] allocated %p [offs: %lu]", ret, pgas_app_addr_offs(ret)); */

  return ret;
}

inline void
pgas_app_free(void* addr)
{
  //  pgas_free_list[++pgas_free_cur] = addr;
  pgas_free_num++;
  /* PRINT("free %3d (num_free after: %3d)", pgas_free_cur, pgas_free_num); */
  pgas_free_list[pgas_free_cur++] = addr;
}

void**
pgas_app_alloc_rr(size_t num_elems, size_t size_elem)
{

  void** mem_rr = (void**) malloc(num_elems * sizeof(void*));
  assert(mem_rr != NULL);

  uint32_t n = pgas_alloc_rr_next;
  size_t alloced_my_node = 0;
  uint32_t i = 0;
  for (i = 0; i < num_elems; i++)
    {
      /* PRINT("allocating from %d", n); */

      mem_rr[i] = PTR_ADD(pgas_app_mem, pgas_allocs[n]);
      pgas_allocs[n] += size_elem;
      if (n == pgas_app_my_resp_node)
	{
	  alloced_my_node += size_elem;
	}
      n = (n + 1) % NUM_DSL_NODES;
      /* PRINT("elem %3d @ %p", i, mem_rr[i]); */
    }

  pgas_alloc_rr_next = n;	/* for using if the function is called again */
  alloc_next += alloced_my_node;
  return mem_rr;
}

inline size_t
pgas_app_addr_offs(void* addr)
{
  return PTR_SUB(addr, pgas_app_mem);
}

inline void*
pgas_app_addr_from_offs(size_t offs)
{
  return PTR_ADD(pgas_app_mem, offs);
}

