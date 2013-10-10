/*
 *   File: fakemem.c
 *   Author: Nikola Knezevic <nikola.knezevic@epfl.ch>
 *   Description: a dummy memory allocator similar to Intel's RCCE
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

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "fakemem.h"

/*#define DEBUG*/
#ifdef DEBUG_FAKEMEM
#include <stdio.h>
#define PRINTD(args...) fprintf(stderr, "fakemem: " args)
#define PRINTMB(text, block) do { \
	fprintf(stderr, "fakemem: %s[%p] =\t" \
			 "{ offset => %zu, size => %zu, next => %p }\n", \
			text, block, block->offset, block->size, block->next); } while (0)
#define PRINTLIST(text, list) do { \
	fakemem_block* tmp_walker_ = list; \
	size_t counter = 0; \
	while (tmp_walker_ != NULL && counter < 10) { \
		fprintf(stderr, "fakemem: %s-%zu[%p] =\t" \
				"{ offset => %zu, size => %zu, next => %p }\n", \
				text, counter, tmp_walker_, tmp_walker_->offset, tmp_walker_->size, tmp_walker_->next); \
		tmp_walker_ = tmp_walker_->next; counter++; }} while (0)
#else
#define PRINTD(args...) do {} while (0)
#define PRINTMB(text, block) do {} while (0)
#define PRINTLIST(text, block) do {} while (0)
#endif // DEBUG


// the size of the memory space presented to the application
#define FAKEMEM_ALLOC_SIZE (512*1024*1024)

// size of the minimal allocation block
// also, the granularity of the allocation
#define FAKEMEM_ALLOC_BLOCK 4


struct _fakemem_block_ {
	size_t offset; // offset from the beginning of the address space
	size_t size;   // size of the block
	struct _fakemem_block_* next; // pointer to the next block
};

typedef struct _fakemem_block_ fakemem_block;

typedef struct {
	void* base_vaddr; // "virtual address" seen by the userspace
	fakemem_block* free_blocks; // list of free blocks, sorted in the increasing order by offset
	fakemem_block* taken; // list of the taken blocks, sorted in the decreasing order by offset (speed, you usualy free the last thing)
} allocator_state;

static allocator_state* FAKEMEM_ALLOC_STATE = NULL;

void* fakemem_init();

void*
fakemem_malloc(size_t size)
{
	// if we are not initialized, we need to allocate the initial data structures
	if (FAKEMEM_ALLOC_STATE == NULL
		&& fakemem_init() == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	if (size == 0) {
		errno = EINVAL;
		return NULL;
	}

	PRINTD("\nmalloc called. CAS = %p, size = %lu\n", FAKEMEM_ALLOC_STATE, size);
	// set the size to be of the granularity
	size = ((size-1)/FAKEMEM_ALLOC_BLOCK+1)*FAKEMEM_ALLOC_BLOCK;
	PRINTD("rounded size is %lu\n", size);

	fakemem_block* walker = FAKEMEM_ALLOC_STATE->free_blocks;
	fakemem_block* parent = walker;
	while (walker != NULL) {
		if (walker->size >= size)
			break;
		if (parent != walker)
			parent = walker;
		walker = walker->next;
	}
	if (walker == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	PRINTD("found walker %p, offset %lu, size %lu\n", walker, walker->offset, walker->size);

	// this is what we put in taken
	fakemem_block* new_block = NULL;

	// first, update the free list
	// new_block will contain the block which needs to go to the taken list
	if (walker->size == size) {
		// we just move the block to the taken list
		new_block = walker;
		if (parent != walker)
			parent->next = walker->next;
		else
			FAKEMEM_ALLOC_STATE->free_blocks = walker->next;
		PRINTD("size matches chunk. parent = %p, new_block = %p\n", parent, new_block);
	} else {
		// we need to create and fill in the new block
		new_block = (fakemem_block*)malloc(sizeof(fakemem_block));
		if (new_block == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		new_block->offset = walker->offset;
		new_block->size = size;

		walker->offset += size;
		walker->size -= size;
		PRINTD("new block created. walker = %p, new_block = %p\n", walker, new_block);
	}
	PRINTD("parent %p, next %p\n", parent, parent->next);

	// then, proceed to adding a new block to the taken list
	walker = FAKEMEM_ALLOC_STATE->taken;
	parent = walker;
	while (walker != NULL) {
		PRINTD("new_block->offset = %lu, walker->offset = %lu\n", new_block->offset, walker->offset);
		if (new_block->offset > walker->offset)
			break;
		if (parent != walker)
			parent = walker;
		walker = walker->next;
	}
	new_block->next = walker;
	PRINTD("taken list: new_block->next = %p\n", new_block->next);
	PRINTD("taken list: parent = %p\n", parent);
	if (parent == NULL || parent == FAKEMEM_ALLOC_STATE->taken)
		FAKEMEM_ALLOC_STATE->taken = new_block;
	else
		parent->next = new_block;
	PRINTD("taken list: parent = %p, parent->next = %p, taken = %p\n", parent, parent?parent->next:parent, FAKEMEM_ALLOC_STATE->taken);

	PRINTLIST("free", FAKEMEM_ALLOC_STATE->free_blocks);
	PRINTLIST("taken", FAKEMEM_ALLOC_STATE->taken);

	PRINTD("malloc ends\n\n");
	// finally, return the address we "allocated"
	return ((void*)((uintptr_t)FAKEMEM_ALLOC_STATE->base_vaddr +
	                new_block->offset));
}

void
fakemem_free(void* ptr)
{
	PRINTD("\nfree called. ptr = %p, CAS = %p, taken = %p\n", ptr, FAKEMEM_ALLOC_STATE, FAKEMEM_ALLOC_STATE->taken);
	if (FAKEMEM_ALLOC_STATE == NULL
		|| FAKEMEM_ALLOC_STATE->taken == NULL)
		return;
	fakemem_block* walker = FAKEMEM_ALLOC_STATE->taken;
	fakemem_block* parent = walker;

	size_t off = (size_t)((uintptr_t)ptr -
	                      (uintptr_t)FAKEMEM_ALLOC_STATE->base_vaddr);
	PRINTD("Offset = %ld\n", off);
	while (walker != NULL) {
		PRINTMB("walker", walker);
		/*PRINTD("walker = %p, walker->next = %p, walker->offset = %lu\n", walker, walker->next, walker->offset);*/
		if (walker->offset == off)
			break;
		if (parent != walker)
			parent = walker;
		walker = walker->next;
	}
	PRINTD("found walker %p\n", walker);
	// this should be an error in a safe language...
	if (walker == NULL)
		return;
	PRINTD("walker is %p, off is %lu\n", walker, walker->offset);

	// remove the node with the right offset
	if (walker == parent && walker->next == NULL) {
		// this is the case with only one...
		FAKEMEM_ALLOC_STATE->taken = NULL;
	} else {
		parent->next = walker->next;
	}
	walker->next = NULL;

	// add to the free list
	fakemem_block* new_block = walker;
	walker = FAKEMEM_ALLOC_STATE->free_blocks;
	parent = walker;

	PRINTLIST("taken", FAKEMEM_ALLOC_STATE->taken);
	// iterate over all free blocks, find the first one which is after out block
	// in: new_block, containing data to enlist
	// out: parent holds the place where to put the data
	while (walker != NULL) {
		PRINTMB("free-walk", walker);
		if (walker->offset > new_block->offset)
			break;
		if (parent != walker)
			parent = walker;
		walker = walker->next;
	}
	if (walker == NULL)
		return;
	PRINTD("walker = %p, parent = %p; parent->size = %lu, offset = %lu\n", walker, parent, parent->size, parent->offset);

	PRINTMB("new_block", new_block);
	PRINTLIST("free_blocks", FAKEMEM_ALLOC_STATE->free_blocks);

	if (parent->offset + parent->size == new_block->offset) {
		PRINTD("Will merge two nodes, parent in front\n");
		// we will assimilate the block;
		parent->size += new_block->size;
		free(new_block);
	} else if (new_block->offset + new_block->size == parent->offset) {
		PRINTD("Will merge two nodes, parent in back\n");
		parent->size += new_block->size;
		parent->offset -= new_block->size;
		free(new_block);
	} else {
		new_block->next = walker;
		if (parent == FAKEMEM_ALLOC_STATE->free_blocks) {
			FAKEMEM_ALLOC_STATE->free_blocks = new_block;
		} else {
			parent->next = new_block;
		}
	}

	// now, do the compacting...
	walker = parent = FAKEMEM_ALLOC_STATE->free_blocks;
	PRINTD("will try to compact...\n");
	while (walker != NULL) {
		PRINTMB("walker", walker);
		/*PRINTD("walker = %p, walker->next = %p, walker->offset = %lu, walker->size = %lu\n", walker, walker->next, walker->offset, walker->size);*/
		if (walker && walker->next) {
			if (walker->offset + walker->size == walker->next->offset) {
				// compact...
				PRINTD("will compact %p\n", walker->next);
				walker->size += walker->next->size;
				fakemem_block* temp = walker->next;
				walker->next = walker->next->next;
				PRINTD("removing %p\n", temp);
				free(temp);
			}
		}
		if (parent != walker)
			parent = walker;
		walker = walker->next;
	}
#ifdef DEBUG
	fakemem_block* taken = FAKEMEM_ALLOC_STATE->taken;
	PRINTD("taken = %p, taken->next = %p\n", taken, taken?taken->next:taken);

	PRINTLIST("free", FAKEMEM_ALLOC_STATE->free_blocks);
	PRINTLIST("taken", FAKEMEM_ALLOC_STATE->taken);

	PRINTD("free ends\n\n");
#endif
}

inline size_t
fakemem_offset(void* ptr)
{
	return (size_t)((uintptr_t)ptr -
	                (uintptr_t)FAKEMEM_ALLOC_STATE->base_vaddr);
}

inline void*
fakemem_addr_from_offset(size_t offset)
{
	return (void*)((uintptr_t)FAKEMEM_ALLOC_STATE->base_vaddr +
	               offset);
}

void*
fakemem_init()
{
	FAKEMEM_ALLOC_STATE = (allocator_state*)malloc(sizeof(allocator_state));
	if (FAKEMEM_ALLOC_STATE == NULL) {
		// terrible error
		errno = ENOMEM;
		return NULL;
	}
	fakemem_block* new_block = (fakemem_block*)malloc(sizeof(fakemem_block));
	if (new_block == NULL) {
		free(FAKEMEM_ALLOC_STATE);
		errno = ENOMEM;
		return NULL;
	}
	new_block->offset = 0;
	new_block->size = FAKEMEM_ALLOC_SIZE;
	new_block->next = NULL;

	FAKEMEM_ALLOC_STATE->base_vaddr = &FAKEMEM_ALLOC_STATE;
	FAKEMEM_ALLOC_STATE->taken = NULL;
	FAKEMEM_ALLOC_STATE->free_blocks = new_block;

	return (void*)FAKEMEM_ALLOC_STATE;
}

