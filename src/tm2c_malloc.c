/*
 *   File: tm2c_malloc.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: a very very simple object allocator
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "tm2c_malloc.h"

#define MAX_FILENAME_LENGTH 100

static void* tm2c_app_mem;
static size_t alloc_next = 0;
static void* tm2c_free_list[256] = {0};
static uint8_t tm2c_free_cur = 0;
static uint8_t tm2c_free_num = 0;

void
tm2c_shmalloc_set(void* mem)
{
  tm2c_app_mem = mem;
}


/* 
   On TILERA we open the shmem in sys_tm2c_init
 */
#if !defined(PLATFORM_TILERA)
//--------------------------------------------------------------------------------------
// FUNCTION: tm2c_shmalloc_init
//--------------------------------------------------------------------------------------
// initialize memory allocator
//--------------------------------------------------------------------------------------
// size (bytes) of managed space
void
tm2c_shmalloc_init(size_t size)
{
   //create the shared space which will be managed by the allocator

   char keyF[MAX_FILENAME_LENGTH];
   sprintf(keyF,"/tm2c_mem2");

   int shmfd = shm_open(keyF, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
   if (shmfd<0)
   {
      if (errno != EEXIST)
      {
         perror("In shm_open");
         exit(1);
      }

      //this time it is ok if it already exists
      shmfd = shm_open(keyF, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
      if (shmfd<0)
      {
         perror("In shm_open");
         exit(1);
      }
   }
   else
   {
      //only if it is just created
     if (!ftruncate(shmfd,size))
       {
	 /* printf("ftruncate failed\n"); */
       }
   }

  void* mem = (void*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  assert(mem != NULL);

  // create one block containing all memory for truly dynamic memory allocator
  tm2c_app_mem = (void*) mem;
}

void
tm2c_shmalloc_term()
{
   char keyF[MAX_FILENAME_LENGTH];
   sprintf(keyF,"/tm2c_mem2");
   shm_unlink(keyF);
}

#else  /* PLATFORM_TILERA */

void
tm2c_shmalloc_init(size_t size)
{
}

void
tm2c_shmalloc_term()
{
}
#endif	/* !PLATFORM_TILERA */

//--------------------------------------------------------------------------------------
void*
tm2c_shmalloc(size_t size)
{
  void *ret;
  if (tm2c_free_num > 2)
    {
      uint8_t spot = tm2c_free_cur - tm2c_free_num;
      ret = tm2c_free_list[spot];
      tm2c_free_num--;
    }
  else
    {
      ret = tm2c_app_mem + alloc_next;
      alloc_next += size;
    }

  /* PRINT("[lib] allocated %p [offs: %lu]", ret, tm2c_app_addr_offs(ret)); */

  return (void*) ret;
}

//--------------------------------------------------------------------------------------
// FUNCTION: tm2c_shfree
//--------------------------------------------------------------------------------------
// Deallocate memory in off-chip shared memory. Also collective, see tm2c_shmalloc
//--------------------------------------------------------------------------------------
// pointer to data to be freed
void
tm2c_shfree(void* ptr)
{
  tm2c_free_num++;
  /* PRINT("free %3d (num_free after: %3d)", tm2c_free_cur, tm2c_free_num); */
  tm2c_free_list[tm2c_free_cur++] = (void*) ptr;
}
