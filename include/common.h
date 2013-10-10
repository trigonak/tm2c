/*
 *   File: common.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: common defines and includes
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
 * File:   common.h
 * Author: trigonak
 *
 * Created on March 30, 2011, 6:15 PM
 */

#ifndef COMMON_H
#define	COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

  //#define PGAS

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>

#ifndef INLINED
#  if __GNUC__ && !__GNUC_STDC_INLINE__ && !SCC
#    define INLINED static inline __attribute__((always_inline))
#  else
#    define INLINED inline
#  endif
#endif

#ifndef EXINLINED
#  if __GNUC__ && !__GNUC_STDC_INLINE__ && !SCC
#    define EXINLINED extern inline
#  else
#    define EXINLINED inline
#  endif
#endif

#ifndef ALIGNED
#  if __GNUC__ && !SCC
#    define ALIGNED(N) __attribute__ ((aligned (N)))
#  else
#    define ALIGNED(N)
#  endif
#endif

#ifndef LLU
#  define LLU long long unsigned int
#endif 

#ifndef LU
#  define LU long unsigned int
#endif 

#ifndef U
#  define U unsigned int
#endif 


#include "tm2c_types.h"

#if defined(PLATFORM_OPTERON)
#  define REF_SPEED_GHZ           2.1
#  define CACHE_LINE_SIZE 64
#elif defined(SCC)
#  define REF_SPEED_GHZ           0.533
/* #  define REF_SPEED_GHZ           0.800 */
#  define CACHE_LINE_SIZE 64
#elif defined(PLATFORM_TILERA)
#  if defined(__tilepro__)
#    define REF_SPEED_GHZ           0.7
#  else
#    define REF_SPEED_GHZ           1.2
#  endif  /* __tilepro__ */
#  define CACHE_LINE_SIZE 64
#elif defined(PLATFORM_NIAGARA)
#  define REF_SPEED_GHZ           1.17
#  define CACHE_LINE_SIZE 16
#elif defined(PLATFORM_XEON)
#  define REF_SPEED_GHZ           2.1
#  define CACHE_LINE_SIZE 64
#else
#  if !defined(REF_SPEED_GHZ)
#    define REF_SPEED_GHZ           2.1
#    warning "Need to set REF_SPEED_GHZ for the platform (defaulted to 2.1 GHz)"
#  endif
#  define CACHE_LINE_SIZE 64
#endif

  typedef enum
    {
      NO_CONFLICT,
      READ_AFTER_WRITE,
      WRITE_AFTER_READ,
      WRITE_AFTER_WRITE,
#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
      PERSISTING_WRITES, 	/* used for contention management */
      TX_COMMITTED,		/* used for contention management */
#endif 				/* NOCM */
    } TM2C_CONFLICT_T;

  /* read or write request
   */
  typedef enum
    {
      READ,
      WRITE
    } RW;

  extern nodeid_t TM2C_ID;
  extern nodeid_t NUM_UES;
  extern nodeid_t NUM_APP_NODES;
  extern nodeid_t NUM_DSL_NODES;
    
  extern int is_app_core(int);
  extern int is_dsl_core(int);

  INLINED nodeid_t
  min_dsl_id() 
  {
    uint32_t i;
    for (i = 0; i < NUM_UES; i++)
      {
	if (!is_app_core(i))
	  {
	    return i;
	  }
      }
    return i;
  }

  /*
    returns the posisition of the core id in the seq of dsl cores
    e.g., having 6 cores total and core 2 and 4 are dsl, then
    the call to this function with node=2=>0, with node=4=>1
  */
  INLINED nodeid_t
  dsl_id_seq(nodeid_t node) 
  {
    uint32_t i, seq = 0;
    for (i = 0; i < node; i++)
      {
	if (!is_app_core(i))
	  {
	    seq++;
	  }
      }
    return seq;
  }

  /*
    returns the posisition of the core id in the seq of dsl cores
    e.g., having 6 cores total and core 2 and 4 are dsl, then
    the call to this function with node=2=>0, with node=4=>1
  */
  INLINED nodeid_t
  app_id_seq(nodeid_t node) 
  {
    uint32_t i, seq = 0;
    for (i = 0; i < node; i++)
      {
	if (is_app_core(i))
	  {
	    seq++;
	  }
      }
    return seq;
  }

  INLINED nodeid_t
  min_app_id() 
  {
    uint32_t i;
    for (i = 0; i < NUM_UES; i++)
      {
	if (is_app_core(i))
	  {
	    return i;
	  }
      }
    return i;
  }

#define ONCE						\
  if ((is_app_core(TM2C_ID) && (NODE_ID() == min_app_id())) ||	\
      TM2C_ID == min_dsl_id() ||				\
      NUM_UES == 1)


#if defined(PLATFORM_TILERA)	/* need it before measurements.h */
  typedef uint64_t ticks;
#endif	/* PLATFORM_TILERA */

#include "measurements.h"

  /*  ------- Plug platform related things here BEGIN ------- */
#if defined(PLATFORM_SCC)	
#  include "RCCE.h"
#  include "sys_scc.h"
#endif 

#if defined(PLATFORM_DEFAULT)
#  include "sys_default.h"
#endif

#if defined(PLATFORM_OPTERON)
#  include "sys_opteron.h"
#endif

#if defined(PLATFORM_XEON)
#  include "sys_xeon.h"
#endif

#if defined(PLATFORM_NIAGARA)
#  include "sys_niagara.h"
#endif

#if defined(PLATFORM_TILERA)
#  include "sys_tilera.h"
#endif

  /*  ------- Plug platform related things here END   ------- */

#define EXIT(reason) exit(reason);

#include "tm2c_sys.h"

  /* ------- Printing related platforms ------- */
#define MED printf("[%02d] ", NODE_ID());
#define PRINT(args...) printf("[%02d] ", NODE_ID()); printf(args); printf("\n"); fflush(stdout)
#define PRINT_ONCE(args...) ONCE { printf("[%02d] ", NODE_ID()); printf(args); printf("\n"); fflush(stdout); }
#define PRINTNF(args...) printf("[%02d] ", NODE_ID()); printf(args); printf("\n"); fflush(stdout)
#define PRINTN(args...) printf("[%02d] ", NODE_ID()); printf(args); fflush(stdout)
#define PRINTS(args...)  printf(args);
#define PRINTSF(args...)  printf(args); fflush(stdout)
#define PRINTSME(args...)  printf("[%02d] ", NODE_ID()); printf(args);

#define FLUSH fflush(stdout);
#ifdef DEBUG
#  define FLUSHD fflush(stdout);
#  define ME printf("%d: ", NODE_ID())
#  define PRINTF(args...) printf(args)
#  define PRINTD(args...) ME; printf(args); printf("\n"); fflush(stdout)
#else
#  define FLUSHD
#  define ME
#  define PRINTF(args...)
#  define PRINTD(args...)
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

