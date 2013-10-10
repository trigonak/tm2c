/*
 *   File: hash.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: hash functions 
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

#ifndef _HASH_H
#define _HASH_H

#include "common.h"

/* Thomas Wang's 32 bit Mix Function */
INLINED uint32_t 
hash_tw(uint32_t key)
{
  key += ~(key << 15);
  key ^=  (key >> 10);
  key +=  (key << 3);
  key ^=  (key >> 6);
  key += ~(key << 11);
  key ^=  (key >> 16);
  return key;
}

/* Fast hashing routine for ints,  longs and pointers.
   (C) 2002 William Lee Irwin III, IBM */
/*
 * Knuth recommends primes in approximately golden ratio to the maximum
 * integer representable by a machine word for multiplicative hashing.
 * Chuck Lever verified the effectiveness of this technique:
 * http://www.citi.umich.edu/techreports/reports/citi-tr-00-1.pdf
 *
 * These primes are chosen to be bit-sparse, that is operations on
 * them can use shifts and additions instead of multiplications for
 * machines where multiplications are slow.
 */

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_32
#define hash_long(val, bits) hash_32(val, bits)

INLINED uint32_t 
hash_32(uint32_t val, uint32_t bits)
{
  /* On some cpus multiply is faster, on others gcc will do shifts */
  uint32_t hash = val * GOLDEN_RATIO_PRIME_32;

  /* High bits are more random, so use them. */
  return hash >> (32 - bits);
}

static inline unsigned long hash_ptr(const void *ptr, unsigned int bits)
{
  return hash_long((unsigned long)ptr, bits);
}

#endif /* _HASH_H */
