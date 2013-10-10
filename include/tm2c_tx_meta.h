/*
 *   File: tm2c_tx_meta.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: transactional metadata
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
 * File: tm2c_tx_meta.h
 * Author: trigonak
 *
 * Created on April 11, 2011, 6:05 PM
 * 
 * Data structures and operations related to the TM metadata
 */

#ifndef _TM3C_TX_META_H
#define	_TM3C_TX_META_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <setjmp.h>
#include "tm2c_log.h"
#include "tm2c_mem.h"

  typedef enum 
    {
      IDLE,
      RUNNING,
      ABORTED,
      COMMITTED
    } TX_STATE;

  typedef struct ALIGNED(64) tm2c_tx /* Transaction descriptor */
  { 
    sigjmp_buf env;		/* Environment for setjmp/longjmp */
#if defined(GREEDY) | defined(FAIRCM) /* placed in diff place than for FAIRCM, according to access seq */
    ticks start_ts;
#endif
    uint32_t aborts;	 /* Total number of aborts (cumulative) */
    uint32_t retries;	  /* Number of consecutive aborts (retries) */
    uint16_t aborts_raw; /* Aborts due to read after write (cumulative) */
    uint16_t aborts_war; /* Aborts due to write after read (cumulative) */
    uint16_t aborts_waw; /* Aborts due to write after write (cumulative) */
    mem_info_t* mem_info; /* Transactional mem alloc lists*/
#if !defined(PGAS)		/* in PGAS only the DSLs hold a write_set */
    tm2c_write_set_t *write_set;	/* Write set */
#endif
  } tm2c_tx_t;

  typedef struct ALIGNED(64) tm2c_tx_node 
  {
    uint32_t tx_starts;
    uint32_t tx_committed;
    uint32_t tx_aborted;
    uint32_t max_retries;
    uint32_t aborts_war;
    uint32_t aborts_raw;
    uint32_t aborts_waw;
#ifdef FAIRCM
    ticks tx_duration;
#else
    uint8_t padding[36];
#endif
  } tm2c_tx_node_t;

  extern void tm2c_tx_meta_node_print(tm2c_tx_node_t * tm2c_tx_node);
  extern void tm2c_tx_meta_print(tm2c_tx_t* tm2c_tx);
  extern tm2c_tx_node_t* tm2c_tx_meta_node_new();
  extern tm2c_tx_t* tm2c_tx_meta_new();
  extern void tm2c_tx_meta_free(tm2c_tx_t **tm2c_tx);

  INLINED tm2c_tx_t* 
  tm2c_tx_meta_empty(tm2c_tx_t *tm2c_tx_temp) 
  {

#if !defined(PGAS)
    tm2c_tx_temp->write_set = write_set_empty(tm2c_tx_temp->write_set);
#endif
    //tm2c_tx_temp->mem_info = mem_info_new();
    //TODO: what about the env?
    tm2c_tx_temp->retries = 0;
    tm2c_tx_temp->aborts = 0;
    tm2c_tx_temp->aborts_war = 0;
    tm2c_tx_temp->aborts_raw = 0;
    tm2c_tx_temp->aborts_waw = 0;
    return tm2c_tx_temp;
  }

#ifdef	__cplusplus
}
#endif

#endif	/* _TM3C_TX_META_H */

