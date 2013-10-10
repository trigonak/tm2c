/*
 *   File: tm2c.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: TM2C interface for applications 
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

/*
 * File:   tm.h
 * Author: trigonak
 *
 * Created on April 13, 2011, 9:58 AM
 * 
 * The TM interface to the user
 */

#ifndef _TM2C_H_
#define	_TM2C_H_

#include <setjmp.h>
#include "common.h"
#include "tm2c_app.h"
#include "tm2c_dsl.h"
#include "tm2c_tx_meta.h"
#include "tm2c_mem.h"
#ifdef PGAS
#  include "pgas_app.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define FOR_ITERS(iters)					\
  double __start = wtime();					\
  uint32_t __iterations;					\
  for (__iterations; __iterations < (iters); __iterations++)	\

#define END_FOR_ITERS				\
  duration__ = wtime() - __start;

#define FOR(seconds)					\
  double __ticks_per_sec = 1000000000*REF_SPEED_GHZ;	\
  ticks __duration_ticks = (seconds) * __ticks_per_sec;	\
  ticks __start_ticks = getticks();			\
  ticks __end_ticks = __start_ticks + __duration_ticks;	\
  while ((getticks()) < __end_ticks) {			\
  uint32_t __reps;					\
  for (__reps = 0; __reps < 1000; __reps++) {

#define END_FOR						\
  }}							\
    __end_ticks = getticks();				\
    __duration_ticks = __end_ticks - __start_ticks;	\
    duration__ = __duration_ticks / __ticks_per_sec;


#define FOR_SEC(seconds)			\
  double __start = wtime();			\
  double __end = __start + (seconds);		\
  while (wtime() < __end) {			\
  uint32_t __reps;				\
  for (__reps = 0; __reps < 100; __reps++) {

#define END_FOR_SEC				\
  }}						\
    __end = wtime();				\
    duration__ = __end - __start;


#define APP_EXEC_ORDER				\
  {						\
  int i;					\
  for (i = 0; i < TOTAL_NODES(); i++)		\
    {						\
  if (i == NODE_ID())				\
    {						
  
#define APP_EXEC_ORDER_END			\
  }						\
    BARRIER;}}

#define DSL_EXEC_ORDER				\
  {						\
  int i;					\
  for (i = 0; i < TOTAL_NODES(); i++)		\
    {						\
  if (i == NODE_ID())				\
    {						
  
#define DSL_EXEC_ORDER_END			\
  }						\
    BARRIER_DSL;}}


  extern tm2c_tx_t* tm2c_tx;
  extern tm2c_tx_node_t* tm2c_tx_node;
  extern double duration__;

  extern const char* conflict_reasons[4];

  /*______________________________________________________________________________
   * Help functions
   *______________________________________________________________________________
   */

  /* 
   * Returns a pseudo-random value in [1;range).
   * Depending on the symbolic constant RAND_MAX>=32767 defined in stdlib.h,
   * the granularity of rand() could be lower-bounded by the 32767^th which might 
   * be too high for given values of range and initial.
   */
  INLINED long 
  rand_range(long int r) 
  {
    int m = RAND_MAX;
    long d, v = 0;
    do 
      {
	d = (m > r ? r : m);
	v += 1 + (long) (d * ((double) rand() / ((double) (m) + 1.0)));
	r -= m;
      } while (r > 0);
    
    return v;
  }



  //Marsaglia's xorshf generator
  //period 2^96-1
  INLINED unsigned long
  tm2c_xorshf96(unsigned long* x, unsigned long* y, unsigned long* z) 
  {
    unsigned long t;
    (*x) ^= (*x) << 16;
    (*x) ^= (*x) >> 5;
    (*x) ^= (*x) << 1;

    t = *x;
    (*x) = *y;
    (*y) = *z;
    (*z) = t ^ (*x) ^ (*y);
    return *z;
  }

  INLINED unsigned long
  tm2c_rand()
  {
    return tm2c_xorshf96(tm2c_rand_seeds, tm2c_rand_seeds + 1, tm2c_rand_seeds + 2);
  }

  INLINED uint32_t
  pow2roundup (uint32_t x)
  {
    if (x==0) 
      {
	return 1;
      }
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
  }


  /*______________________________________________________________________________________________________
   * TM Interface                                                                                         |
   *______________________________________________________________________________________________________|
   */


#define TM2C_INIT				\
  tm2c_init_system(&argc, &argv);		\
  {						\
  tm2c_init();

#define TM2C_INITs				\
  {						\
  tm2c_init();


  extern unsigned long* seed_rand();

#define SEQ_INIT				\
  tm2c_init_system(&argc, &argv);		\
  sys_app_init();				\
  tm2c_rand_seeds = seed_rand();

#ifdef PGAS
#  define WSET                  NULL
#  define WSET_EMPTY
#  define WSET_PERSIST(tm2c_tx)
#else
#  define WSET                  tm2c_tx->write_set
#  define WSET_EMPTY           WSET = write_set_empty(WSET)
#  define WSET_PERSIST(tm2c_tx) write_set_persist(tm2c_tx)
#endif

#ifdef EAGER_WRITE_ACQ
#  define WLOCKS_ACQUIRE()
#  define WLOCK_ACQUIRE(addr)    tx_wlock(addr, 0)
#else
#  define WLOCKS_ACQUIRE()       tm2c_rpc_store_all()
#  define WLOCK_ACQUIRE(addr)
#endif

  /* -------------------------------------------------------------------------------- */
  /* Contention management related macros */
  /* -------------------------------------------------------------------------------- */

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
#  define TXRUNNING()       set_tx_running();

#  if defined(PGAS)
#    define TXCOMMITTED()   
#    define TXCOMPLETED()   set_tx_committed();
#  else	/* !PGAS */
#    define TXCOMMITTED()   set_tx_committed();
#    define TXCOMPLETED()   
#  endif	/* PGAS */

#  define TXPERSISTING()			\
  if (!set_tx_persisting())			\
    {						\
      TX_ABORT(get_abort_reason());		\
    }

#  define TXCHKABORTED()			\
  if (check_aborted())				\
    {						\
      TX_ABORT(get_abort_reason());		\
    }
#else  /* if no CM or BACKOFF_RETRY */
#  define TXRUNNING()     ;
#  define TXCOMMITTED()   ;
#  define TXCOMPLETED()   ;
#  define TXPERSISTING()  ;
#  define TXCHKABORTED()  ;
#endif	/* NOCM */

#if defined(WHOLLY) || defined(NOCM) || defined(BACKOFF_RETRY)
#  define CM_METADATA_INIT_ON_START            ;
#  define CM_METADATA_INIT_ON_FIRST_START      ;
#  define CM_METADATA_UPDATE_ON_COMMIT         ;
#elif defined(FAIRCM)
#  define CM_METADATA_INIT_ON_START            tm2c_tx->start_ts = getticks();
#  define CM_METADATA_INIT_ON_FIRST_START      ;
#  define CM_METADATA_UPDATE_ON_COMMIT         tm2c_tx_node->tx_duration += (getticks() - tm2c_tx->start_ts);
#elif defined(GREEDY)
#  define CM_METADATA_INIT_ON_START            ;
#  ifdef GREEDY_GLOBAL_TS	/* use fetch and increment */
#    define CM_METADATA_INIT_ON_FIRST_START      tm2c_tx->start_ts = greedy_get_global_ts();
#  else
#    define CM_METADATA_INIT_ON_FIRST_START      tm2c_tx->start_ts = getticks();
#  endif
#  define CM_METADATA_UPDATE_ON_COMMIT         ;
#else  /* no cm defined */
#  define CM_METADATA_INIT_ON_START            ;
#  define CM_METADATA_INIT_ON_FIRST_START      ;
#  define CM_METADATA_UPDATE_ON_COMMIT         ;
#  define NOCM
#  warning "One of the contention managers should be selected. Using NOCM as default."
#endif


#define TX_START					\
  { PRINTD("|| Starting new tx");			\
  CM_METADATA_INIT_ON_FIRST_START;			\
  short int reason;					\
  if ((reason = sigsetjmp(tm2c_tx->env, 0)) != 0) {	\
    PRINTD("|| restarting due to %d", reason);		\
    WSET_EMPTY;						\
  }							\
  tm2c_tx->retries++;					\
  TXRUNNING();						\
  CM_METADATA_INIT_ON_START;

#define TX_ABORT(reason)			\
  PRINTD("|| aborting tx (%d)", reason);	\
  tm2c_handle_abort(tm2c_tx, reason);		\
  siglongjmp(tm2c_tx->env, reason);

#define TX_COMMIT				\
  WLOCKS_ACQUIRE();				\
  TXPERSISTING();				\
  WSET_PERSIST(tm2c_tx->write_set);		\
  TXCOMMITTED();				\
  tm2c_rpc_rls_all(NO_CONFLICT);		\
  TXCOMPLETED();				\
  CM_METADATA_UPDATE_ON_COMMIT;			\
  tm2c_tx_node->tx_starts++;			\
  tm2c_tx_node->tx_committed++;			\
  tm2c_tx_node->tx_aborted += tm2c_tx->aborts;	\
  tm2c_tx = tm2c_tx_meta_empty(tm2c_tx);}


#define TX_COMMIT_MEM				\
  WLOCKS_ACQUIRE();				\
  TXPERSISTING();				\
  WSET_PERSIST(tm2c_tx->write_set);		\
  TXCOMMITTED();				\
  tm2c_rpc_rls_all(NO_CONFLICT);		\
  TXCOMPLETED();				\
  CM_METADATA_UPDATE_ON_COMMIT;			\
  mem_info_on_commit(tm2c_tx->mem_info);	\
  tm2c_tx_node->tx_starts++;			\
  tm2c_tx_node->tx_committed++;			\
  tm2c_tx_node->tx_aborted += tm2c_tx->aborts;	\
  tm2c_tx = tm2c_tx_meta_empty(tm2c_tx);}


  /* #define TX_COMMIT					\ */
  /*   tm2c_tx_node->max_retries =				\ */
  /*     (tm2c_tx->retries < tm2c_tx_node->max_retries)	\ */
  /*     ? tm2c_tx_node->max_retries				\ */
  /*     : tm2c_tx->retries;					\ */
  /*   tm2c_tx_node->aborts_war += tm2c_tx->aborts_war;	\ */
  /*   tm2c_tx_node->aborts_raw += tm2c_tx->aborts_raw;	\ */
  /*   tm2c_tx_node->aborts_waw += tm2c_tx->aborts_waw;	\ */


#define TX_COMMIT_NO_STATS			\
  WLOCKS_ACQUIRE();				\
  TXPERSISTING();				\
  WSET_PERSIST(tm2c_tx->write_set);		\
  TXCOMMITTED();				\
  tm2c_rpc_rls_all(NO_CONFLICT);		\
  TXCOMPLETED();				\
  CM_METADATA_UPDATE_ON_COMMIT;			\
  mem_info_on_commit(tm2c_tx->mem_info);	\
  tm2c_tx_node->tx_committed++;			\
  tm2c_tx = tm2c_tx_meta_empty(tm2c_tx);}

#define TX_COMMIT_NO_PUB_NO_STATS		\
  TXPERSISTING();				\
  WSET_PERSIST(tm2c_tx->write_set);		\
  TXCOMMITTED();				\
  tm2c_rpc_rls_all(NO_CONFLICT);		\
  TXCOMPLETED();				\
  CM_METADATA_UPDATE_ON_COMMIT;			\
  mem_info_on_commit(tm2c_tx->mem_info);	\
  tm2c_tx = tm2c_tx_meta_empty(tm2c_tx);}

#define TX_COMMIT_NO_PUB			\
  TXPERSISTING();				\
  WSET_PERSIST(tm2c_tx->write_set);		\
  TXCOMMITTED();				\
  tm2c_rpc_rls_all(NO_CONFLICT);		\
  TXCOMPLETED();				\
  CM_METADATA_UPDATE_ON_COMMIT;			\
  tm2c_tx_node->tx_starts += tm2c_tx->retries;	\
  tm2c_tx_node->tx_committed++;			\
  tm2c_tx_node->tx_aborted += tm2c_tx->aborts;	\
  tm2c_tx = tm2c_tx_meta_empty(tm2c_tx); }


#define TM_END					\
  tm2c_rpc_stats(tm2c_tx_node, duration__);	\
  TM2C_TERM;}

#define TM_EXIT(code)				\
  tm2c_rpc_stats(tm2c_tx_node, duration__);	\
  TM2C_TERM;					\
  exit(code);

#define TM_END_STATS				\
  tm2c_rpc_stats(tm2c_tx_node, duration__);	\
  tm2c_tx_meta_free(&tm2c_tx);			\
  tm2c_tx_meta_node_print(tm2c_tx_node);	\
  free(tm2c_tx_node); }                                  

#define TM2C_TERM				\
  tm2c_term();					\
  term_system();

  /*
   * addr is the local address
   */
#if defined(PGAS)
#  define TX_LOAD(addr, words)			\
  tx_load(WSET, (addr), words) 
#else
#  define TX_LOAD(addr)				\
  tx_load(WSET, (addr)) 
#endif	/* PGAS */

#define TX_LOAD_T(addr,type) (type)TX_LOAD(addr)

  /*
   * addr is the local address
   * NONTX acts as a regular load on iRCCE w/o PGAS
   * and fetches data from remote on all other systems
   */
#if defined(PGAS)
#  define NONTX_LOAD(addr, words)		\
  nontx_load(addr, words)
#else  /* !PGAS */
#  define NONTX_LOAD(addr)			\
  nontx_load(addr)
#endif	/* PGAS */


#define NONTX_LOAD_T(addr,type) (type)NONTX_LOAD(addr)

#if defined(PGAS)
#  define TX_STORE(addr, val, datatype)		\
  do {						\
    TXCHKABORTED();				\
    tx_wlock(addr, val);			\
  } while (0)
#else /* !PGAS */
#  define TX_STORE(addr, val, datatype)				\
  do {								\
    WLOCK_ACQUIRE((addr));					\
    TXCHKABORTED();						\
    tm_intern_addr_t intern_addr = to_intern_addr((addr));	\
    write_set_insert(tm2c_tx->write_set,			\
		     datatype,					\
		     val, intern_addr);				\
  } while (0)
#endif

#define TX_CAS(addr, oldval, newval)		\
  tx_cas(addr, oldval, newval);

#define TX_CASI(addr, oldval, newval)		\
  tx_casi(addr, oldval, newval);


#if defined(PGAS)
#  define NONTX_STORE(addr, val, datatype)	\
  nontx_store(addr, val)
#else 
#  define NONTX_STORE(addr, val, datatype)	\
  *((__typeof__ (val)*)(addr)) = (val)
#endif

#ifdef PGAS
#  define TX_LOAD_STORE(addr, op, value, datatype)	\
  do { tx_store_inc(addr, op(value)); } while (0)
#else
#  define TX_LOAD_STORE(addr, op, value, datatype)			\
  do {									\
    tx_wlock(addr, 0);							\
    int temp__ = (*(int* ) (addr)) op (value);				\
    tm_intern_addr_t intern_addr = to_intern_addr((tm_addr_t)addr);	\
    write_set_insert(tm2c_tx->write_set, TYPE_INT, temp__, intern_addr); \
  } while (0)
#endif

#define DUMMY_MSG(to)				\
  tm2c_rpc_dummy(to)
  


  /*early release of READ lock -- TODO: the entry remains in read-set, so one
    SHOULD NOT try to re-read the address cause the tx things it keeps the lock*/
#define TX_RRLS(addr)				\
  tm2c_rpc_load_rls((void*) (addr));

#define TX_WRLS(addr)				\
  tm2c_rpc_store_rls((void*) (addr));

  /*__________________________________________________________________________________________
   * TRANSACTIONAL MEMORY ALLOCATION
   * _________________________________________________________________________________________
   */

#define TX_MALLOC(size)				\
  stm_malloc(tm2c_tx->mem_info, (size_t) size)

#define TX_SHMALLOC(size)				\
  stm_shmalloc(tm2c_tx->mem_info, (size_t) size)

#define TX_FREE(addr)				\
  stm_free(tm2c_tx->mem_info, (void* ) addr)

#define TX_SHFREE(addr)				\
  stm_shfree(tm2c_tx->mem_info, (void*) addr)

  void tm2c_handle_abort(tm2c_tx_t* tm2c_tx, TM2C_CONFLICT_T reason);

  /*
   * API function
   * accepts the machine-local address, which must be translated to the appropriate
   * internal address
   */
#if defined(PGAS)
  INLINED int64_t
  tx_load(tm2c_write_set_pgas_t* ws, tm_addr_t addr, int words)
  {
    TM2C_CONFLICT_T conflict;

    TXCHKABORTED();
    if ((conflict = tm2c_rpc_load(addr, words)) != NO_CONFLICT) 
      {
	TX_ABORT(conflict);
      }

    return read_value;
  }

#else  /* !PGAS */

  INLINED tm_addr_t 
  tx_load(tm2c_write_set_t* ws, tm_addr_t addr)
  {
    tm_intern_addr_t intern_addr = to_intern_addr(addr);
#  if defined(PREFETCH_LOAD)
    PREFETCH(intern_addr);
#  endif
    write_entry_t* we;
    if ((we = write_set_contains(ws, intern_addr)) != NULL) 
      {
	return (void* ) &we->i;
      }
    else 
      {
	//the node is NOT already subscribed for the address
	TM2C_CONFLICT_T conflict;
	TXCHKABORTED();
	if ((conflict = tm2c_rpc_load(addr, 0)) != NO_CONFLICT) /* 0 for the #words used in PGAS */
	  {
	    TX_ABORT(conflict);
	  }
	/* TXCHKABORTED(); */
	return addr;
      }
  }
#endif	/* PGAS */
  /*  get a tx write lock for address addr
   */
  INLINED
  void tx_wlock(tm_addr_t address, int64_t value)
  {
    TM2C_CONFLICT_T conflict;
    TXCHKABORTED();
    if ((conflict = tm2c_rpc_store(address, value)) != NO_CONFLICT)
      {
	TX_ABORT(conflict);
      }
  }

  /*
   * The non-transactional load
   */
#ifdef PGAS
  INLINED int64_t
  nontx_load(tm_addr_t addr, unsigned int words) 
  {
    int64_t v = tm2c_rpc_notx_load(addr, words);
    return v;
  }
#else /* !PGAS */
  INLINED tm_addr_t 
  nontx_load(tm_addr_t addr) 
  {
    return addr;
  }
#endif /* PGAS */

  /*
   * The non transactional store, only in PGAS version
   */
  INLINED void 
  nontx_store(tm_addr_t addr, int64_t value) 
  {
    return tm2c_rpc_notx_store(addr, value);
  }

#if defined(PGAS)
  INLINED void
  tx_store_inc(tm_addr_t address, int64_t value)
  {
    TM2C_CONFLICT_T conflict;
    TXCHKABORTED();
    if ((conflict = tm2c_rpc_store_inc(address, value)) != NO_CONFLICT)
      {
	TX_ABORT(conflict);
      }
  }
#endif  /* PGAS */


  extern void tm2c_init_system(int* argc, char** argv[]);
  extern void tm2c_init(void);
  extern void tm2c_term(void);


#ifdef	__cplusplus
}
#endif

#endif	/* _TM2C_H_ */

