/*
 *   File: pgas_app.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: interface for PGAS for app processes
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

#ifndef PGAS_APP_H
#define	PGAS_APP_H

#include "common.h"
#include "pgas_dsl.h"

extern nodeid_t pgas_app_my_resp_node;
extern nodeid_t pgas_app_my_resp_node_real;
extern size_t pgas_dsl_size_node;

extern void pgas_app_init();
extern void pgas_app_term();

extern size_t pgas_app_addr_offs(void* addr);
extern void* pgas_app_addr_from_offs(size_t offs);


extern void* pgas_app_alloc(size_t size);
extern void** pgas_app_alloc_rr(size_t num_elems, size_t size_elem);
extern void pgas_app_free(void* addr);

#endif	/* PGAS_APP_H */
