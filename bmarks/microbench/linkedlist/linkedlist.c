/*
 * Adapted to TM2C by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch> 
 *
 * File:
 *   test.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Concurrent accesses of the linked list
 *
 * Copyright (c) 2009-2010.
 *
 * test.c is part of Microbench
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

#include "linkedlist.h"

nxt_t offs__;

void* shmem_init(size_t offset)
{
  return (void *) (sys_shmalloc(offset) + offset);
}

node_t*
new_node(val_t val, nxt_t next, int transactional) 
{
  node_t *node;

  if (transactional) 
    {
      node = (node_t *) TX_SHMALLOC(sizeof (node_t));
    }
  else 
    {
      node = (node_t *) sys_shmalloc(sizeof (node_t));
    }
  if (node == NULL)
    {
      perror("malloc");
      EXIT(1);
    }

  node->val = val;
  node->next = next;

  return node;
}

intset_t*
set_new() 
{
  intset_t *set;
  node_t *min, *max;

  if ((set = (intset_t *) sys_shmalloc(sizeof (intset_t))) == NULL) 
    {
      perror("malloc");
      EXIT(1);
    }
  max = new_node(VAL_MAX, 0, 0);
  min = new_node(VAL_MIN, OF(max), 0);
  set->head = OF(min);
  return set;
}

void
set_delete(intset_t *set) 
{
  node_t *node, *next;

  node = ND(set->head);
  while (node != NULL) {
    next = ND(node->next);
    sys_shfree((void*) node);
    node = next;
  }
  sys_shfree((void*) set);
}

int set_size(intset_t *set) {
    int size = 0;
    node_t *node, *head;

    /* We have at least 2 elements */
    head = ND(set->head);
    node = ND(head->next);
    while (node->nextp != NULL) {
        size++;
        node = ND(node->next);
    }

    return size;
}

/*
 *  intset.c
 *  
 *  Integer set operations (contain, insert, delete) 
 *  that call stm-based / lock-free counterparts.
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

int
set_contains(intset_t *set, val_t val, int transactional) 
{
  int result;

#ifdef DEBUG
  printf("[%02d]++> set_contains(%d)\n", NODE_ID(), (int) val);
  printf("before: "); set_print(set);
  FLUSH;
#endif

#ifdef SEQUENTIAL
  node_t *prev, *next;

#  ifdef LOCKS
  global_lock();
#  endif

  prev = ND(set->head);
  next = ND(prev->next);
  while (next->val < val) {
    prev = next;
    next = ND(prev->next);
  }
  result = (next->val == val);

#  ifdef LOCKS
  global_lock_release();
#  endif

#elif defined STM

#  ifndef READ_VALIDATION
  node_t *prev, *next;
#    ifdef EARLY_RELEASE
  node_t *rls;
#    endif
  val_t v = 0;

  TX_START;
  prev = ND(set->head);
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));
  while (1) {
    v = next->val;
    if (v >= val)
      break;
#    ifdef EARLY_RELEASE
    rls = prev;
#    endif
    prev = next;
    next = ND(*(nxt_t *) TX_LOAD(&prev->next));
#    ifdef EARLY_RELEASE
    TX_RRLS(&rls->next);
#    endif
  }
  FLUSH;
  TX_COMMIT;
  result = (v == val);

#  else	 /* READ_VALIDATION */
  node_t *prev, *next, *validate;
  nxt_t nextoffs, validateoffs;
  val_t v = 0;

  TX_START;
  prev = ND(set->head);
  nextoffs = prev->next;
  next = ND(nextoffs);
  validate = prev;
  validateoffs = nextoffs;
  while (1) {
    v = next->val;
    if (v >= val)
      break;
    validate = prev;
    validateoffs = nextoffs;
    prev = next;
    nextoffs = prev->next;
    next = ND(nextoffs);
    if (validate->next != validateoffs) {
      PRINTD("[C1] Validate failed: expected nxt: %d, got %d", validateoffs, validate->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
  }
  if (validate->next != validateoffs) {
    PRINTD("[C2] Validate failed: expected nxt: %d, got %d", validateoffs, validate->next);
    TX_ABORT(READ_AFTER_WRITE);
  }
  TX_COMMIT;
  result = (v == val);
#  endif
#endif	

  return result;
}

static int set_seq_add(intset_t *set, val_t val) {
    int result;
    node_t *prev, *next;

#ifdef LOCKS
    global_lock();
#endif

    prev = ND(set->head);
    next = ND(prev->next);
    while (next->val < val) {
        prev = next;
        next = ND(prev->next);
    }
    result = (next->val != val);
    if (result) {
        prev->next = OF(new_node(val, OF(next), 0));
    }


#ifdef LOCKS
    global_lock_release();
#endif

    return result;
}

int
set_add(intset_t *set, val_t val, int transactional) 
{
  int result = 0;

#ifdef DEBUG
  printf("[%02d]++> set_add(%d)\n", NODE_ID(), (int) val);
  printf("before: "); set_print(set);
  FLUSH;
#endif

  if (!transactional) {
    return set_seq_add(set, val);
  }

#ifdef SEQUENTIAL /* Unprotected */

  result = set_seq_add(set, val);

#elif defined STM
#  ifndef READ_VALIDATION
  node_t *prev, *next;

#    ifdef EARLY_RELEASE
  node_t *prls, *pprls;
#    endif
  val_t v;
  TX_START;
  prev = ND(set->head);
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));

  v = next->val;
  if (v >= val)
    goto done;

#    ifdef EARLY_RELEASE
  pprls = prev;
#    endif
  prev = next;
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));

#    ifdef EARLY_RELEASE
  prls = prev;
#    endif
  while (1) {
    v = next->val;
    if (v >= val)
      break;
    prev = next;
    next = ND(*(nxt_t *) TX_LOAD(&prev->next));
#    ifdef EARLY_RELEASE
    TX_RRLS(&pprls->next);
    pprls = prls;
    prls = prev;
#    endif
  }
 done:
  result = (v != val);
  if (result) {
    nxt_t nxt = OF(new_node(val, OF(next), transactional));
    PRINTD("Created node %5d. Value: %d", nxt, val);
    TX_STORE(&prev->next, nxt, TYPE_INT64);
  }
  TX_COMMIT_MEM;

#    ifdef DEBUG
  printf("[%02d]--> set_add(%d)\n", NODE_ID(), (int) val);
  printf("after: "); set_print(set);
  FLUSH;
#    endif


#  else	 /* READ_VALIDATION */
  node_t *prev, *next, *validate, *pvalidate;
  nxt_t nextoffs, validateoffs, pvalidateoffs;

  val_t v;
  TX_START;

  prev = ND(set->head);

  nextoffs = prev->next;
  next = ND(nextoffs);

  pvalidate = prev;
  pvalidateoffs = validateoffs = nextoffs;

  v = next->val;
  if (v >= val)
    {
      goto done;
    }

  prev = next;
  nextoffs = prev->next;
  next = ND(nextoffs);

  validate = prev;
  validateoffs = nextoffs;

  while (1) {
    v = next->val;
    if (v >= val)
      break;
    prev = next;
    nextoffs = prev->next;
    next = ND(nextoffs);
    if (pvalidate->next != pvalidateoffs) {
      PRINTD("[A1] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
    pvalidate = validate;
    pvalidateoffs = validateoffs;
    validate = prev;
    validateoffs = nextoffs;
  }
 done:
  result = (v != val);
  if (result) {
    if (&pvalidate->next != &prev->next)
      {
	TX_LOAD(&pvalidate->next);
      }
    if ((*(nxt_t *) TX_LOAD(&prev->next)) != validateoffs) {
      PRINTD("[A2] Validate failed: expected nxt: %d, got %d", validateoffs, prev->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
    nxt_t nxt = OF(new_node(val, OF(next), transactional));
    PRINTD("Created node %5d. Value: %d", nxt, val);
    TX_STORE(&prev->next, nxt, TYPE_INT64);

    if (pvalidate->next != pvalidateoffs) {
      PRINTD("[A3] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
  }

  TX_COMMIT_MEM;

#    ifdef DEBUG
  printf("[%02d]--> set_add(%d)\n", NODE_ID(), (int) val);
  printf("after: "); set_print(set);
  FLUSH;
#    endif

#  endif
#endif
  return result;
}

int set_remove(intset_t *set, val_t val, int transactional) 
{
  int result = 0;

#ifdef DEBUG
  printf("[%02d]++> set_remove(%d)\n", NODE_ID(), (int) val);
  printf("before: "); set_print(set);
  FLUSH;
#endif

#ifdef SEQUENTIAL /* Unprotected */

  node_t *prev, *next;

#  ifdef LOCKS
  global_lock();
#  endif

  prev = ND(set->head);
  next = ND(prev->next);
  while (next->val < val) {
    prev = next;
    next = ND(prev->next);
  }
  result = (next->val == val);
  if (result) {
    prev->next = next->next;
    sys_shfree((void*) next);
  }

#  ifdef LOCKS
  global_lock_release();
#  endif

#elif defined STM
#  ifndef READ_VALIDATION
  node_t *prev, *next;
#    ifdef EARLY_RELEASE
  node_t *prls, *pprls;
#    endif
  val_t v;

  TX_START;
  prev = ND(set->head);
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));

  //v = *(val_t *) TX_LOAD(&next->val);
  v = next->val;
  if (v >= val)
    goto done;

#    ifdef EARLY_RELEASE
  pprls = prev;
#    endif

  prev = next;
  next = ND(*(nxt_t *) TX_LOAD(&prev->next));
#    ifdef EARLY_RELEASE
  prls = prev;
#    endif

  while (1) {
    v = next->val;
    if (v >= val)
      break;
    prev = next;
    next = ND(*(nxt_t *) TX_LOAD(&prev->next));
#    ifdef EARLY_RELEASE
    TX_RRLS(&pprls->next);
    pprls = prls;
    prls = prev;
#    endif
  }
 done:
  result = (v == val);
  if (result) {
    nxt_t nxt = *(nxt_t *) TX_LOAD(&next->next);
    TX_STORE(&prev->next, nxt, TYPE_INT64);
    TX_SHFREE(next);
    PRINTD("Freed node   %5d. Value: %d", OF(next), next->val);
  }
  TX_COMMIT_MEM;

#    ifdef DEBUG
  printf("[%02d]--> set_remove(%d)\n", NODE_ID(), (int) val);
  printf("after: "); set_print(set);
  FLUSH;
#    endif

#  else	 /* READ_VALIDATION */
  node_t *prev, *next, *validate, *pvalidate;
  nxt_t nextoffs, validateoffs, pvalidateoffs;
  val_t v;

  TX_START;
  prev = ND(set->head);
  nextoffs = prev->next;
  next = ND(nextoffs);

  pvalidate = prev;
  pvalidateoffs = validateoffs = nextoffs;

  v = next->val;
  if (v >= val)
    {
      PRINTD("** (rem) first element in the list");
      goto done;
    }

  prev = next;
  nextoffs = prev->next;
  next = ND(nextoffs);

  validate = prev;
  validateoffs = nextoffs;

  while (1) {
    v = next->val;
    if (v >= val)
      break;
    prev = next;
    nextoffs = prev->next;
    next = ND(nextoffs);
    if (pvalidate->next != pvalidateoffs) {
      PRINTD("[R1] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
    pvalidate = validate;
    pvalidateoffs = validateoffs;
    validate = prev;
    validateoffs = nextoffs;
  }
 done:
  result = (v == val);
  if (result) {
    if (&pvalidate->next != &prev->next)
      {
	TX_LOAD(&pvalidate->next);
      }

    if ((*(nxt_t *) TX_LOAD(&prev->next)) != validateoffs) {
      PRINTD("[R2] Validate failed: expected nxt: %d, got %d", validateoffs, prev->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
    nxt_t nxt = *(nxt_t *) TX_LOAD(&next->next);
    TX_STORE(&prev->next, nxt, TYPE_INT64);
    TX_SHFREE(next);
    if (pvalidate->next != pvalidateoffs) {
      PRINTD("[R3] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next);
      TX_ABORT(READ_AFTER_WRITE);
    }
  }
  TX_COMMIT_MEM;
#  endif
#endif
  return result;
}

void set_print(intset_t* set) 
{

  node_t *node = ND(set->head);
  node = ND(node->next);

  if (node == NULL) 
    {
      goto null;
    }

  while (node->nextp != NULL) 
    {
      printf("(@%p) %u -[%u]-> ", &node->next, (unsigned int) node->val, (unsigned int) node->next);
      node = ND(node->next);
    }

 null:
  PRINTSF("NULL\n");

}
