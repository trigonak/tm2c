/*
 *   File: tm2c_log.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: write log for transactions
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
 * File: tm2c_log.h
 * Author: trigonak
 *
 * Created on March 31, 2011, 2:27 PM
 */

#ifndef _TM2C_LOG_H
#define	_TM2C_LOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "common.h"

#define CAST_INT(addr) *(addr)

  typedef enum 
    {
      TYPE_INT,
      TYPE_INT64
    } DATATYPE;


  /*______________________________________________________________________________________________________
   * WRITE SET                                                                                             |
   *______________________________________________________________________________________________________|
   */


#define WRITE_SET_SIZE 4

  typedef struct write_entry 
  {
    tm_intern_addr_t address;
    DATATYPE type;
    int32_t i;
  } write_entry_t;

  typedef struct write_set 
  {
    write_entry_t* write_entries;
    uint32_t nb_entries;
    uint32_t size;
  } tm2c_write_set_t;

  extern tm2c_write_set_t* write_set_new();

  extern void write_set_free(tm2c_write_set_t* write_set);

  extern tm2c_write_set_t* write_set_empty(tm2c_write_set_t* write_set);

  inline write_entry_t* write_set_entry(tm2c_write_set_t* write_set);

  inline void write_entry_set_value(write_entry_t* we, int32_t value);

  extern void write_set_insert(tm2c_write_set_t* write_set, DATATYPE datatype, int32_t value, tm_intern_addr_t address);

  extern void write_set_update(tm2c_write_set_t* write_set, DATATYPE datatype, int32_t value, tm_intern_addr_t address);

  extern void write_entry_persist(write_entry_t* we);

  extern void write_entry_print(write_entry_t* we);

  extern void write_set_print(tm2c_write_set_t* write_set);

  extern void write_set_persist(tm2c_write_set_t* write_set);

  extern write_entry_t* write_set_contains(tm2c_write_set_t* write_set, tm_intern_addr_t address);

#ifdef PGAS

#  include "pgas_dsl.h"


  /*______________________________________________________________________________________________________
   * WRITE SET PGAS                                                                                       |
   *______________________________________________________________________________________________________|
   */


#define WRITE_SET_PGAS_SIZE     4

  typedef struct write_entry_pgas 
  {
    tm_intern_addr_t address;
    int64_t value;
  } write_entry_pgas_t;

  typedef struct write_set_pgas 
  {
    write_entry_pgas_t *write_entries;
    uint32_t nb_entries;
    uint32_t size;
  } tm2c_write_set_pgas_t;

  extern tm2c_write_set_pgas_t* write_set_pgas_new();

  extern void write_set_pgas_free(tm2c_write_set_pgas_t *write_set_pgas);

  extern void write_set_pgas_empty(tm2c_write_set_pgas_t *write_set_pgas);

  inline write_entry_pgas_t * write_set_pgas_entry(tm2c_write_set_pgas_t *write_set_pgas);

  extern void write_set_pgas_insert(tm2c_write_set_pgas_t *write_set_pgas, int64_t value, tm_intern_addr_t address);

  extern void write_set_pgas_update(tm2c_write_set_pgas_t *write_set_pgas, int64_t value, tm_intern_addr_t address);

  inline void write_entry_pgas_persist(write_entry_pgas_t *we);

  inline void write_entry_pgas_print(write_entry_pgas_t *we);

  extern void write_set_pgas_print(tm2c_write_set_pgas_t *write_set_pgas);

  extern uint32_t write_set_pgas_persist(tm2c_write_set_pgas_t *write_set_pgas);

  extern write_entry_pgas_t * write_set_pgas_contains(tm2c_write_set_pgas_t *write_set_pgas, tm_intern_addr_t address);


#endif

#endif	/* _TM2C_LOG_H */

