/*
 *   File: tm2c_cm.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: contention management interface
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

#ifndef _TM2C_CM_H
#define _TM2C_CM_H

#include "common.h"

typedef struct 
{
  union 
  {
    uint64_t num_txs;
    ticks timestamp;
    double duration;
  };
} cm_metadata_t;
extern cm_metadata_t *cm_metadata_core;

extern int32_t* cm_init();
extern void cm_term(nodeid_t node);

#if defined(GREEDY) && defined(GREEDY_GLOBAL_TS)
static ticks* cm_greedy_global_ts_init();
inline ticks greedy_get_global_ts();
#endif


extern BOOLEAN 
contention_manager(nodeid_t attacker, unsigned short *defenders, TM2C_CONFLICT_T conflict);

extern BOOLEAN 
contention_manager_raw_waw(nodeid_t attacker, unsigned short defender, TM2C_CONFLICT_T conflict);

extern BOOLEAN 
contention_manager_war(nodeid_t attacker, uint8_t *defenders, TM2C_CONFLICT_T conflict);

void
contention_manager_pri_print(void);
#endif /* _TM2C_CM_H */
