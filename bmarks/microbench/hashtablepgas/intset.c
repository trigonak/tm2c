/*
 * Adapted to TM2C by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch> 
 *
 * File:
 *   intset.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Integer set operations accessing the hashtable
 *
 * Copyright (c) 2009-2010.
 *
 * intset.c is part of Microbench
 * 
 * Microbench is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "intset.h"

intset_t *offset;

int 
ht_contains(ht_intset_t* set, int val, int transactional) 
{
  int addr, result;

  addr = val % maxhtlength;
  result = set_contains(set->buckets[addr], val, transactional);

  return result;
}

int 
ht_add(ht_intset_t* set, int val, int transactional) 
{
  int addr, result;

  addr = val % maxhtlength;
  result = set_add(set->buckets[addr], val, transactional);

  return result;
}

int 
ht_remove(ht_intset_t* set, int val, int transactional) 
{
  int addr, result;

  addr = val % maxhtlength;
  result = set_remove(set->buckets[addr], val, transactional);

  return result;
}
