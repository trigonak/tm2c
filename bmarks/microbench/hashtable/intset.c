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

extern int range;

int
ht_contains(ht_intset_t *set, int val, int transactional) 
{
  int addr, result;

  addr = val % maxhtlength;
  if (transactional == 5)
    result = set_contains(set->buckets[addr], val, 4);
  else
    result = set_contains(set->buckets[addr], val, transactional);

  return result;
}

int
ht_add(ht_intset_t *set, int val, int transactional) 
{
  int result;

  int addr = val % maxhtlength;
  result = set_add(set->buckets[addr], val, transactional);
  return result;
}

int
ht_remove(ht_intset_t *set, int val, int transactional) 
{
  int result;
  int addr = val % maxhtlength;
  result = set_remove(set->buckets[addr], val, transactional);
  return result;

}

/* 
 * Move an element from one bucket to another.
 * It is equivalent to changing the key associated with some value.
 *
 * This version allows the removal of val1 while insertion of val2 fails (e.g.
 * because val2 already present. (Despite this partial failure, the move returns 
 * true.) As a result, data structure size may decrease as moves execute.
 */
int
ht_move_naive(ht_intset_t *set, int val1, int val2, int transactional) 
{
  int result = 0;

#ifdef SEQUENTIAL

#ifdef LOCKS
  global_lock();
#endif

  int addr1, addr2;

  addr1 = val1 % maxhtlength;
  addr2 = val2 % maxhtlength;
  result = (set_remove(set->buckets[addr1], val1, transactional) &&
            set_add(set->buckets[addr2], val2, transactional));

#ifdef LOCKS
  global_lock_release();
#endif

#elif defined STM

  int v, addr1, addr2;
  node_t *prev, *next, *prev1, *next1;
  nxt_t nxt;

  addr1 = val1 % maxhtlength;
  addr2 = val2 % maxhtlength;
  while (addr1 == addr2)
    {
      val2 = (val2 + 1) % range;
      addr2 = val2 % maxhtlength;
    }


  TX_START;
  result = 0;

  OFFSET(set->buckets[addr1]);
  prev = ND(set->buckets[addr1]->head);
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));

  while (1) 
    {
      v = next->val; //was TX
      if (v >= val1) 
	{
	  break;
	}
      prev = next;
      next = ND(*(nxt_t *) TX_LOAD(&prev->next));
    }

  if (v == val1) 
    {
      result = 2;
      /* Physically removing */
      nxt = *(nxt_t *) TX_LOAD(&next->next);
      TX_STORE(&prev->next, nxt, TYPE_INT64);
      TX_SHFREE(next);
      /* Inserting */
      OFFSET(set->buckets[addr2]);
      prev1 = ND(set->buckets[addr2]->head);
      next1 = ND(*(nxt_t *) TX_LOAD(&prev1->next));
      while (1) 
	{
	  v = next1->val; //was TX
	  if (v >= val2) 
	    {
	      break;
	    }
	  prev1 = next1;
	  next1 = ND(*(nxt_t *) TX_LOAD(&prev1->next));
	}
      if (v != val2) 
	{
	  nxt_t nxt1 = OF(new_node(val2, OF(next1), transactional));
	  //PRINTD("Created node %5d. Value: %d", nxt, val);
	  TX_STORE(&prev1->next, nxt1, TYPE_INT64);
	  result = 1;
	}
      /* Even if the key is already in, the operation succeeds */
    }
  /* else  */
  /*   { */
  /*     result = 0; */
  /*   } */
  TX_COMMIT_MEM;

#endif

    return result;
}

/*
 * This version parses the data structure twice to find appropriate values 
 * before updating it.
 */
int ht_move(ht_intset_t *set, int val1, int val2, int transactional) {
  int result = 0;

#ifdef SEQUENTIAL

  int addr1, addr2;

#ifdef LOCKS
  global_lock();
#endif

  addr1 = val1 % maxhtlength;
  addr2 = val2 % maxhtlength;
  //result =  (set_remove(set->buckets[addr1], val1, transactional) && 
  //		   set_add(set->buckets[addr2], val2, transactional));

  if (set_remove(set->buckets[addr1], val1, 0))
    result = 1;
  set_add(set->buckets[addr2], val2, 0);

#ifdef LOCKS
  global_lock_release();
#endif

  return result;

#elif defined STM

  int v, addr1, addr2;
  node_t *prev, *next, *prev1, *next1;

  addr1 = val1 % maxhtlength;
  addr2 = val2 % maxhtlength;
  while (addr1 == addr2)
    {
      val2 = (val2 + 1) % range;
      addr2 = val2 % maxhtlength;
    }

  TX_START;

  result=0;

  OFFSET(set->buckets[addr1]);
  prev = ND(set->buckets[addr1]->head);
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));

  while (1)
    {
      v = next->val; //was TX
      if (v >= val1)
	{
	  break;
	}
      prev = next;
      next = ND(*(nxt_t *) TX_LOAD(&prev->next));
    }

  prev1 = prev;
  next1 = next;
  if (v == val1)
    {
      /* Inserting */
      OFFSET(set->buckets[addr2]);
      prev = ND(set->buckets[addr2]->head);
      next = ND(*(nxt_t *) TX_LOAD(&prev->next));

      while (1)
	{
	  v = next->val; //was TX
	  if (v >= val2)
	    {
	      break;
	    }
	  prev = next;
	  next = ND(*(nxt_t *) TX_LOAD(&prev->next));
	}

      if (v != val2 && prev != prev1 && prev != next1)
	{
	  /* Even if the key is already in, the operation succeeds */
	  result = 1;

	  /* Physically removing */
	  nxt_t nxt1 = *(nxt_t *) TX_LOAD(&next1->next);
	  TX_STORE(&prev1->next, nxt1, TYPE_INT);
	  nxt_t nxt2 = OF(new_node(val2, OF(next), transactional));
	  TX_STORE(&prev->next, nxt2, TYPE_INT);
	  TX_SHFREE(next1);
	}
    }
  TX_COMMIT_MEM;

#endif

  return result;
}

/*
 * This version removes val1 it finds first and re-insert this value if it does 
 * not succeed in inserting val2 so that it can insert somewhere (for the size 
 * to remain unchanged).
 */
int ht_move_orrollback(ht_intset_t *set, int val1, int val2, int transactional) {
    int result = 0;

#ifdef SEQUENTIAL

    int addr1, addr2;


#ifdef LOCKS
    global_lock();
#endif 

    addr1 = val1 % maxhtlength;
    addr2 = val2 % maxhtlength;
    result = (set_remove(set->buckets[addr1], val1, transactional) &&
            set_add(set->buckets[addr2], val2, transactional));

#ifdef LOCKS
    global_lock_release();
#endif

#elif defined STM

    int v, addr1, addr2;
    node_t *prev, *next, *prev1, *next1;

    TX_START
    addr1 = val1 % maxhtlength;
    OFFSET(set->buckets[addr1]);
    prev = ND(*(nxt_t *) TX_LOAD(&set->buckets[addr1]->head));
    next = ND(*(nxt_t *) TX_LOAD(&prev->next));
    while (1) {
        v = next->val; //was TX
        if (v >= val1) {
            break;
        }
        prev = next;
        next = ND(*(nxt_t *) TX_LOAD(&prev->next));
    }
    prev1 = prev;
    next1 = next;
    if (v == val1) {
        /* Physically removing */
        nxt_t nxt = *(nxt_t *) TX_LOAD(&next->next);
        TX_STORE(&prev->next, nxt, TYPE_INT);
        /* Inserting */
        addr2 = val2 % maxhtlength;
        OFFSET(set->buckets[addr2]);
        prev = ND(*(nxt_t *) TX_LOAD(&set->buckets[addr2]->head));
        next = ND(*(nxt_t *) TX_LOAD(&prev->next));
        while (1) {
            v = next->val; //was TX
            if (v >= val2) {
                break;
            }
            prev = next;
            next = ND(*(nxt_t *) TX_LOAD(&prev->next));
        }
        if (v != val2) {
            nxt_t nxt = OF(new_node(val2, OF(next), transactional));
            //PRINTD("Created node %5d. Value: %d", nxt, val);
            TX_STORE(&prev->next, nxt, TYPE_INT);
            TX_SHFREE(next1);
        }
        else {
            nxt_t nxt = *(nxt_t *) TX_LOAD(&next1);
            TX_STORE(&prev1->next, nxt, TYPE_INT);
        }
        /* Even if the key is already in, the operation succeeds */
        result = 1;
    }
    else result = 0;
    TX_COMMIT

#endif

    return result;
}

/*
 * Atomic snapshot of the hash table.
 * It parses the whole hash table to sum all elements.
 *
 * Observe that this particular operation (atomic snapshot) cannot be implemented using 
 * elastic transactions in combination with the move operation, however, normal transactions
 * compose with elastic transactions.
 */
int ht_snapshot(ht_intset_t *set, int transactional) {
    int result = 0;

#ifdef SEQUENTIAL


#ifdef LOCKS
    global_lock();
#endif

    int i, sum = 0;
    node_t *next;

    for (i = 0; i < maxhtlength; i++) {
        OFFSET(set->buckets[i]);
        node_t *hd = ND(set->buckets[i]->head);
        next = ND(hd->next);
        while (next->nextp) {
            sum += next->val;
            next = ND(next->next);
        }
    }
    result = 1;

#ifdef LOCKS
    global_lock_release();
#endif

#elif defined STM

    int i, sum;
    node_t *next;

    TX_START
    sum = 0;
    for (i = 0; i < maxhtlength; i++) {
        OFFSET(set->buckets[i]);
        node_t *hd = ND(set->buckets[i]->head);
        next = ND(*(nxt_t *) TX_LOAD(&hd->next));
        while (next->next) {
            //sum += TX_LOAD(&next->val);
            sum += next->val;
            next = ND(*(nxt_t *) TX_LOAD(&next->next));
        }
    }
    TX_COMMIT
    result = 1;

#elif defined LOCKFREE /* No CAS-based implementation is provided */

    printf("ht_snapshot: No other implementation of atomic snapshot is available\n");
    exit(1);

#endif

    return result;
}
