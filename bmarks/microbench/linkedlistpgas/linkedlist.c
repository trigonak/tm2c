/*
 * PGAS version
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

/* #define DEBUG */

node_t*
new_node(val_t val, nxt_t next, int transactional) 
{
  node_t *node;

  node_t nd;
  nd.val = val;
  nd.next = next;

  if (transactional) 
    {
      node = (node_t *) TX_SHMALLOC(sizeof (node_t));
      TX_STORE(node, nd.to_int64, TYPE_INT);
    }
  else 
    {
      node = (node_t *) sys_shmalloc(sizeof (node_t));
      NONTX_STORE(node, nd.to_int64, TYPE_INT);
    }
  if (node == NULL) {
    perror("malloc");
    EXIT(1);
  }

  return node;
}

static void
write_node(node_t* node, val_t val, nxt_t next, int transactional) 
{
  node_t nd;
  nd.val = val;
  nd.next = next;
  if (transactional)
    {
      TX_STORE(node, nd.to_int64, TYPE_INT);
    }
  else
    {
      NONTX_STORE(node, nd.to_int64, TYPE_INT);
    }
}

intset_t*
set_new() 
{
  intset_t *set;
  node_t *min, *max;

  if ((set = (intset_t *) malloc(sizeof (intset_t))) == NULL) 
    {
      perror("malloc");
      EXIT(1);
    }

  node_t** nodes = (node_t**) pgas_app_alloc_rr(2, sizeof(node_t));
  min = nodes[0];
  max = nodes[1];
  write_node(max, VAL_MAX, 0, 0);
  write_node(min, VAL_MIN, OF(max), 0);

  set->head = min;
  return set;
}

void set_delete(intset_t *set) {
    node_t node, next;

    LOAD_NODE(node, set->head);
    nxt_t to_del = OF(set->head);
    while (node.next != 0) {
        to_del = node.next;
        LOAD_NODE(next, ND(node.next));
        sys_shfree(ND(to_del));
        node.next = next.next;
    }
    sys_shfree((void*) set);
}

int
set_size(intset_t* set) 
{
  int size = 0;
  node_t node, head;

  /* We have at least 2 elements */
  LOAD_NODE(head, set->head);
  LOAD_NODE(node, ND(head.next));
  while (node.next != 0)
    {
       size++;
      LOAD_NODE(node, ND(node.next));
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

/*
  set contains --------------------------------------------------

 */
//static int set_seq_contains(intset_t *set, int val);
//static int set_early_contains(intset_t *set, int val);
//static int set_readval_contains(intset_t *set, int val);

int 
set_contains(intset_t *set, val_t val, int transactional) 
{
  int result;

#ifdef DEBUG
  printf("++> set_contains(%d)\n", (int) val);
  FLUSH;
#endif

#ifdef SEQUENTIAL
  return set_seq_contains(set, val);
#endif
#ifdef EARLY_RELEASE
  return set_early_contains(set, val):
#endif

#ifdef READ_VALIDATION
    return set_readval_contains(set, val);
#endif

  node_t prev, next;

  TX_START;
  TX_LOAD_NODE(prev, set->head);
  TX_LOAD_NODE(next, ND(prev.next));
  while (next.val < val) 
    {
      prev.next = next.next;
      TX_LOAD_NODE(next, ND(prev.next));
    }
  TX_COMMIT;
  result = (next.val == val);

  return result;
}

#ifdef SEQUENTIAL
static int 
set_seq_contains(intset_t *set, val_t val) 
{
  int result;

  node_t prev, next;

#ifdef LOCKS
  global_lock();
#endif

  /* We have at least 2 elements */
  LOAD_NODE(prev, set->head);
  LOAD_NODE(next, ND(prev.next));
  while (next.val < val) 
    {
      prev.next = next.next;
      LOAD_NODE(next, ND(prev.next));
    }

  result = (next.val == val);

#ifdef LOCKS
  global_lock_release();
#endif

  return result;
}
#endif /* SEQUENTIAL */

/* static int set_early_contains(intset_t *set, val_t val) { */
/*   int result; */
/*   node_t prev, next; */
/* #ifdef EARLY_RELEASE */
/*     node_t *rls; */
/* #endif */
/*     val_t v = 0; */

/*     TX_START */
/*     LOAD_NODE_NXT(prev, set->head); */
/*     TX_LOAD_NODE(next, prev.next); */
/*     while (1) { */
/*         v = next.val; */
/*         if (v >= val) */
/*             break; */
/* #ifdef EARLY_RELEASE */
/*         rls = prev; */
/* #endif */
/*         prev.next = next.next; */
/*         TX_LOAD_NODE(next, prev.next); */
/* #ifdef EARLY_RELEASE */
/*         TX_RRLS(&rls->next); */
/* #endif */
/*     } */
/*     TX_COMMIT */
/*     result = (v == val); */

/*     return result; */
/* } */

/* static int set_readval_contains(intset_t *set, val_t val) { */
/*   int result; */

/*   node_t *prev, *next, *validate; */
/*   nxt_t nextoffs, validateoffs; */
/*   val_t v = 0; */

/*   TX_START */
/*     prev = ND(set->head); */
/*   nextoffs = prev->next; */
/*   next = ND(nextoffs); */
/*   validate = prev; */
/*   validateoffs = nextoffs; */
/*   while (1) { */
/*     v = next->val; */
/*     if (v >= val) */
/*       break; */
/*     validate = prev; */
/*     validateoffs = nextoffs; */
/*     prev = next; */
/*     nextoffs = prev->next; */
/*     next = ND(nextoffs); */
/*     if (validate->next != validateoffs) { */
/*       PRINTD("[C1] Validate failed: expected nxt: %d, got %d", validateoffs, validate->next); */
/*       TX_ABORT(READ_AFTER_WRITE); */
/*     } */
/*   } */
/*   if (validate->next != validateoffs) { */
/*     PRINTD("[C2] Validate failed: expected nxt: %d, got %d", validateoffs, validate->next); */
/*     TX_ABORT(READ_AFTER_WRITE); */
/*   } */
/*   TX_COMMIT */
/*     result = (v == val); */

/*   return result; */
/* } */



/*
  set add ----------------------------------------------------------------------                                
 */

static int set_seq_add(intset_t *set, val_t val);
/*static int set_early_add(intset_t *set, val_t val);
static int set_readval_add(intset_t *set, val_t val);
*/

int 
set_add(intset_t* set, val_t val, int transactional) 
{
  int result = 0;

  if (!transactional)
    {
      return set_seq_add(set, val);
    }

#ifdef SEQUENTIAL /* Unprotected */
  return set_seq_add(set, val);
#endif
#ifdef EARLY_RELEASE
  return set_early_add(set, val);
#endif

#ifdef READ_VALIDATION
  return set_readval_add(set, val);
#endif
  node_t prev, next;
  nxt_t to_store;

  TX_START;

#ifdef DEBUG
  PRINT("++> set_add(%d)\tretry: %u", (int) val, tm2c_tx->retries);
#endif

  to_store = OF(set->head);
  TX_LOAD_NODE(prev, set->head);
  TX_LOAD_NODE(next, ND(prev.next));
  while (next.val < val) 
    {
      to_store = prev.next;
      prev.val = next.val;
      prev.next = next.next;
      TX_LOAD_NODE(next, ND(prev.next));
    }
  result = (next.val != val);
  if (result) 
    {
      node_t* nn = new_node(val, prev.next, 1);
      prev.next = OF(nn);
      TX_STORE(ND(to_store), prev.to_int64, TYPE_INT);
    }
  TX_COMMIT_MEM;
  return result;
}


static int 
set_seq_add(intset_t* set, val_t val) 
{
  int result;
  node_t prev, nnext;
#ifdef LOCKS
  global_lock();
#endif

  nxt_t to_store = OF(set->head);

  /* int seq = 0; */
  node_t* hd = set->head;
  LOAD_NODE(prev, hd);

  /* PRINT("%3d   LOAD: head: %10lu   val: %10ld   %d", seq++, to_store, prev.val, prev.next); */
  node_t* nd = ND(prev.next);
  LOAD_NODE(nnext, nd);
  /* PRINT("%3d   LOAD: addr: %10d   val: %10ld   %d", seq++, prev.next, nnext.val, nnext.next); */

  while (nnext.val < val) 
    {
      to_store = prev.next;
      prev.val = nnext.val;
      prev.next = nnext.next;
      node_t* nd = ND(prev.next);
      LOAD_NODE(nnext, nd);
      /* PRINT("%3d   LOAD: addr: %10lu   val: %10ld   %d", seq++, prev.next, nnext.val, nnext.next); */
    }
  result = (nnext.val != val);
  if (result) 
    {
      node_t *nn = new_node(val, prev.next, 0);
      prev.next = OF(nn);
      node_t* nd = ND(to_store);
      NONTX_STORE(nd, prev.to_int64, TYPE_INT);
      /* PRINT("%3d  STORE: addr: %10lu   val: %10ld   %d", seq++, to_store, prev.val, prev.next); */
    }

#ifdef LOCKS
  global_lock_release();
#endif

  return result;
}

/* int set_early_add(intset_t *set, int val) { */
/*   int result; */

/*     node_t prev, next; */

/* #ifdef EARLY_RELEASE */
/*     node_t *prls, *pprls; */
/* #endif */
/*     val_t v; */
/*     TX_START */
/*     nxt_t to_store = set->head + sizeof(sys_t_vcharp); */
/*     LOAD_NODE_NXT(prev, set->head); */
/*     LOAD_NODE(next, prev.next); */

/*     v = next.val; */
/*     if (v >= val) { */
/*         goto done; */
/*     } */

/* #ifdef EARLY_RELEASE */
/*     pprls = prev; */
/* #endif */

/*     to_store = prev.next + sizeof(sys_t_vcharp); */
/*     prev.next = next.next; */
/*     TX_LOAD_NODE(next, prev.next); */

/* #ifdef EARLY_RELEASE */
/*     prls = prev; */
/* #endif */
/*     while (1) { */
/*       v = next.val; */
/*       if (v >= val) { */
/* 	break; */
/*       } */

/*       to_store = prev.next + sizeof(sys_t_vcharp); */
/*       prev.next = next.next; */
/*       LOAD_NODE(next, prev.next); */
/* #ifdef EARLY_RELEASE */
/*         TX_RRLS(&pprls->next); */
/*         pprls = prls; */
/*         prls = prev; */
/* #endif */
/*     } */

/* done: */
/*     result = (v != val); */
/*     if (result) { */
      
/*       node_t *nn = new_node(val, prev.next, 1); */
/*       //      PRINT("Created node %5d. Value: %d", I(nn), val); */
/*       TX_STORE((tm_addr_t)to_store, (int) nn, TYPE_INT); */
/*     } */
/*     TX_COMMIT */
      
/*       return result; */
/* } */

/* int set_readval_add(intset_t *set, int val) { */
/*   int result; */

/*   node_t *prev, *next, *validate, *pvalidate; */
/*     nxt_t nextoffs, validateoffs, pvalidateoffs; */

/*     val_t v; */
/*     TX_START; */
/*     prev = ND(set->head); */
/*     nextoffs = prev->next; */
/*     next = ND(nextoffs); */

/*     pvalidate = prev; */
/*     pvalidateoffs = validateoffs = nextoffs; */

/*     v = next->val; */
/*     if (v >= val) */
/*         goto done; */

/*     prev = next; */
/*     nextoffs = prev->next; */
/*     next = ND(nextoffs); */

/*     validate = prev; */
/*     validateoffs = nextoffs; */

/*     while (1) { */
/*         v = next->val; */
/*         if (v >= val) */
/*             break; */
/*         prev = next; */
/*         nextoffs = prev->next; */
/*         next = ND(nextoffs); */
/*         if (pvalidate->next != pvalidateoffs) { */
/*             PRINTD("[A1] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next); */
/*             TX_ABORT(READ_AFTER_WRITE); */
/*         } */
/*         pvalidate = validate; */
/*         pvalidateoffs = validateoffs; */
/*         validate = prev; */
/*         validateoffs = nextoffs; */
/*     } */
/* done: */
/*     result = (v != val); */
/*     if (result) { */
/*       TX_LOAD(&pvalidate->next, 1); */
/*       if ((*(nxt_t *) TX_LOAD(&prev->next, 1)) != validateoffs) { */
/*             PRINTD("[A2] Validate failed: expected nxt: %d, got %d", validateoffs, prev->next); */
/*             TX_ABORT(READ_AFTER_WRITE); */
/*         } */
/*         //fixnxt_t nxt = OF(new_node(val, OF(next), transactional)); */
/*         PRINTD("Created node %5d. Value: %d", nxt, val); */
/*         //TX_STORE(&prev->next, &nxt, TYPE_UINT); */
/*         if (pvalidate->next != pvalidateoffs) { */
/*             PRINTD("[A3] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next); */
/*             TX_ABORT(READ_AFTER_WRITE); */
/*         } */
/*     } */
/*     TX_COMMIT */


/*   return result; */
/* } */


/*
  set remove --------------------------------------------------------------

 */
int set_seq_remove(intset_t *set, val_t val);
int set_early_remove(intset_t *set, val_t val);
int set_readval_remove(intset_t *set, val_t val);

int
set_remove(intset_t* set, val_t val, int transactional) 
{
  int result = 0;

#ifdef DEBUG
  PRINT("++> set_remove(%d)", (int) val);
#endif

#ifdef SEQUENTIAL /* Unprotected */
  return set_seq_remove(set, val);
#endif
#ifdef EARLY_RELEASE
  return set_early_remove(set, val);
#endif
#ifdef READ_VALIDATION
  return set_readval_remove(set, val);
#endif

  node_t prev, next;

  TX_START;

  nxt_t to_store = OF(set->head);
  TX_LOAD_NODE(prev, set->head);
  TX_LOAD_NODE(next, ND(prev.next));
  while (val > next.val) 
    {
      to_store = prev.next;
      prev.val = next.val;
      prev.next = next.next;
      TX_LOAD_NODE(next, ND(prev.next));
    }
  result = (next.val == val);
  if (result) 
    {
      TX_SHFREE(ND(prev.next));
      prev.next = next.next;
      TX_STORE(ND(to_store), prev.to_int64, TYPE_INT);
    }
  TX_COMMIT_MEM;
  return result;
}

/* static int set_seq_remove(intset_t *set, val_t val) { */
/*   int result; */

/*   node_t prev, next; */

/* #ifdef LOCKS */
/*   global_lock(); */
/* #endif */
/*   nxt_t to_store = set->head + sizeof(sys_t_vcharp); */
/*   LOAD_NODE_NXT(prev, set->head); */
/*   LOAD_NODE(next, prev.next); */
/*   while (next.val < val) { */
/*     to_store = prev.next + sizeof(sys_t_vcharp); */
/*     prev.next = next.next; */
/*     LOAD_NODE(next, prev.next); */
/*   } */
/*   result = (next.val == val); */
/*   if (result) { */
/*     NONTX_STORE((tm_addr_t) to_store, prev.next, TYPE_INT); */
/*     sys_shfree((t_vcharp) prev.next); */
/*   } */

/* #ifdef LOCKS */
/*   global_lock_release(); */
/* #endif */

/*   return result; */
/* } */

/* static int set_early_remove(intset_t *set, val_t val) { */
/*   int result; */

/*   node_t prev, next; */
/* #ifdef EARLY_RELEASE */
/*   node_t *prls, *pprls; */
/* #endif */
/*   val_t v; */

/*   TX_START */
/*     nxt_t to_store = set->head + sizeof(sys_t_vcharp); */
/*   TX_LOAD_NODE_NXT(prev, set->head); */
/*   TX_LOAD_NODE(next, prev.next); */

/*   v = next.val; */
/*   if (v >= val) { */
/*     goto done; */
/*   } */

/* #ifdef EARLY_RELEASE */
/*   pprls = prev; */
/* #endif */
/*   to_store = prev.next + sizeof(sys_t_vcharp); */
/*   prev.next = next.next; */
/*   TX_LOAD_NODE(next, prev.next); */

/* #ifdef EARLY_RELEASE */
/*   prls = prev; */
/* #endif */

/*   while (1) { */
/*     v = next.val; */
/*     if (v >= val) { */
/*       break; */
/*     } */

/*     to_store = prev.next + sizeof(sys_t_vcharp); */
/*     prev.next = next.next; */
/*     LOAD_NODE(next, prev.next); */
/* #ifdef EARLY_RELEASE */
/*     TX_RRLS(&pprls->next); */
/*     pprls = prls; */
/*     prls = prev; */
/* #endif */
/*   } */

/*  done: */
/*   result = (v == val); */
/*   if (result) { */
/*     TX_STORE((tm_addr_t) to_store, next.next, TYPE_INT); */
/*     TX_SHFREE((t_vcharp) prev.next); */
/*     //      PRINT("Freed node   %5d. Value: %d", I(prev.next), next.val); */
/*   } */

/*   TX_COMMIT */

/*     return result; */
/* } */

/* static int set_readval_remove(intset_t *set, val_t val) { */
/*   int result; */

/*   node_t *prev, *next, *validate, *pvalidate; */
/*   nxt_t nextoffs, validateoffs, pvalidateoffs; */
/*   val_t v; */

/*   TX_START */
/*     prev = ND(set->head); */
/*   nextoffs = prev->next; */
/*   next = ND(nextoffs); */

/*   pvalidate = prev; */
/*   pvalidateoffs = validateoffs = nextoffs; */

/*   v = next->val; */
/*   if (v >= val) */
/*     goto done; */

/*   prev = next; */
/*   nextoffs = prev->next; */
/*   next = ND(nextoffs); */

/*   validate = prev; */
/*   validateoffs = nextoffs; */

/*   while (1) { */
/*     v = next->val; */
/*     if (v >= val) */
/*       break; */
/*     prev = next; */
/*     nextoffs = prev->next; */
/*     next = ND(nextoffs); */
/*     if (pvalidate->next != pvalidateoffs) { */
/*       PRINTD("[R1] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next); */
/*       TX_ABORT(READ_AFTER_WRITE); */
/*     } */
/*     pvalidate = validate; */
/*     pvalidateoffs = validateoffs; */
/*     validate = prev; */
/*     validateoffs = nextoffs; */
/*   } */
/*  done: */
/*   result = (v == val); */
/*   if (result) { */
/*     TX_LOAD(&pvalidate->next, 1); */
/*     if ((*(nxt_t *) TX_LOAD(&prev->next, 1)) != validateoffs) { */
/*       PRINTD("[R2] Validate failed: expected nxt: %d, got %d", validateoffs, prev->next); */
/*       TX_ABORT(READ_AFTER_WRITE); */
/*     } */
/*     nxt_t *nxt = (nxt_t *) TX_LOAD(&next->next, 1); */
/*     TX_STORE(&prev->next, (int) nxt, TYPE_UINT); */
/*     TX_SHFREE(next); */
/*     if (pvalidate->next != pvalidateoffs) { */
/*       PRINTD("[R3] Validate failed: expected nxt: %d, got %d", pvalidateoffs, pvalidate->next); */
/*       TX_ABORT(READ_AFTER_WRITE); */
/*     } */
/*   } */
/*   TX_COMMIT */

/*   return result; */
/* } */

/*
  set print ----------------------------------------------------------------------

 */

void 
set_print(intset_t* set) 
{

  node_t node;
  LOAD_NODE(node, set->head);
  /* printf("min -%d-> ", node.next); */
 printf("min --> ");
  LOAD_NODE(node, pgas_app_addr_from_offs(node.next));
  if (node.next == 0) 
    {
      goto null;
    }
  while (node.next != 0) 
    {
      /* printf("%d -%d-> ", node.val, node.next); */
      printf("%d --> ", node.val);
      LOAD_NODE(node, pgas_app_addr_from_offs(node.next));
    }

 null:
  PRINTSF("max -> NULL\n");

}
