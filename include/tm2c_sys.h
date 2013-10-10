/*
 *   File: tm2c_sys.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: platform specific funtions
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

#ifndef TM2C_SYS_H
#define TM2C_SYS_H

#include "tm2c_types.h"

/*
 * This method is called to initialize all necessary things
 * immediately upon startup
 */
void sys_tm2c_init_system(int* argc, char** argv[]);

/*
 * This method is called by tm2c_init_system to initialize the barrier
 */
void tm2c_init_barrier();

/*
 * This method is called to terminate everything set up in tm2c_init_system called
 */
void term_system();

/*
 * Allocate memory from the shared pool. Every node shares this memory.
 *
 * On RCCE that is the main shared memory. On cluster, we fake it...
 */

void* sys_shmalloc(size_t);
void sys_shfree(void*);

/*
 * Functions to deal with the number of nodes.
 */
EXINLINED nodeid_t NODE_ID(void);
INLINED nodeid_t TOTAL_NODES(void);

/*
 * Various helper initialization/termination functions
 * (sys_X is called from X initialization/termination function)
 */
void sys_tm2c_init(void);
void sys_app_init(void);
void sys_dsl_init(void);

void sys_dsl_term(void);
void sys_app_term(void);

/*
 * messaging related functions
 */
INLINED int sys_sendcmd(void* data, size_t len, nodeid_t to);
INLINED int sys_recvcmd(void* data, size_t len, nodeid_t from);
INLINED int sys_is_processed(nodeid_t to);

INLINED int sys_sendcmd_all(void* data, size_t len);

/*
 * to_intern_addr takes the address as used by the client, and transforms it to
 * the internal address (address recognized by the server code, that holds data).
 * to_addr does the opposite, takes the internal address, and transforms it to
 * the machine address.
 */
INLINED tm_intern_addr_t to_intern_addr(tm_addr_t addr);
EXINLINED tm_addr_t to_addr(tm_intern_addr_t i_addr);

/*
 * Random numbers related functions
 */
void srand_core();

/*
 * Helper functions
 */
void wait_cycles(uint64_t ncycles);
void udelay(uint64_t micros);
void ndelay(uint64_t micros);

INLINED double wtime(void);

#endif
