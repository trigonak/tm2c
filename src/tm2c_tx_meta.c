/*
 *   File: tm2c_tx_meta.c
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

#include "tm2c_tx_meta.h"

void
tm2c_tx_meta_node_print(tm2c_tx_node_t * tm2c_tx_node) 
{
  printf("TXs Statistics for node --------------------------------------\n");
  printf("Starts      \t: %u\n", tm2c_tx_node->tx_starts);
  printf("Commits     \t: %u\n", tm2c_tx_node->tx_committed);
  printf("Aborts      \t: %u\n", tm2c_tx_node->tx_aborted);
  printf("Max Retries \t: %u\n", tm2c_tx_node->max_retries);
  printf("Aborts WAR  \t: %u\n", tm2c_tx_node->aborts_war);
  printf("Aborts RAW  \t: %u\n", tm2c_tx_node->aborts_raw);
  printf("Aborts WAW  \t: %u\n", tm2c_tx_node->aborts_waw);
  printf("--------------------------------------------------------------\n");
  fflush(stdout);
}

void
tm2c_tx_meta_print(tm2c_tx_t* tm2c_tx) 
{
  printf("TX Statistics ------------------------------------------------\n");
  printf("Retries     \t: %u\n", tm2c_tx->retries);
  printf("Aborts      \t: %u\n", tm2c_tx->aborts);
  printf("Aborts WAR  \t: %u\n", tm2c_tx->aborts_war);
  printf("Aborts RAW  \t: %u\n", tm2c_tx->aborts_raw);
  printf("Aborts WAW  \t: %u\n", tm2c_tx->aborts_waw);
  printf("--------------------------------------------------------------\n");
  fflush(stdout);
}

tm2c_tx_node_t*
tm2c_tx_meta_node_new() 
{
  tm2c_tx_node_t *tm2c_tx_node_temp = (tm2c_tx_node_t *) malloc(sizeof (tm2c_tx_node_t));
  if (tm2c_tx_node_temp == NULL) 
    {
      printf("malloc tm2c_tx_node @ tm2c_tx_meta_node_new");
      return NULL;
    }

  tm2c_tx_node_temp->tx_starts = 0;
  tm2c_tx_node_temp->tx_committed = 0;
  tm2c_tx_node_temp->tx_aborted = 0;
  tm2c_tx_node_temp->max_retries = 0;
  tm2c_tx_node_temp->aborts_war = 0;
  tm2c_tx_node_temp->aborts_raw = 0;
  tm2c_tx_node_temp->aborts_waw = 0;

#if defined(FAIRCM)
  tm2c_tx_node_temp->tx_duration = 1;
#endif

  return tm2c_tx_node_temp;
}

tm2c_tx_t* 
tm2c_tx_meta_new() 
{
  tm2c_tx_t *tm2c_tx_temp = (tm2c_tx_t *) malloc(sizeof(tm2c_tx_t));
  if (tm2c_tx_temp == NULL) 
    {
      printf("malloc tm2c_tx @ tm2c_tx_meta_new");
      return NULL;
    }

#if !defined(PGAS) 
  tm2c_tx_temp->write_set = write_set_new();
#endif
  tm2c_tx_temp->mem_info = mem_info_new();

  tm2c_tx_temp->retries = 0;
  tm2c_tx_temp->aborts = 0;
  tm2c_tx_temp->aborts_war = 0;
  tm2c_tx_temp->aborts_raw = 0;
  tm2c_tx_temp->aborts_waw = 0;
  /* tm2c_tx_temp->max_retries = 0; */

#if defined(FAIRCM) || defined(GREEDY)
  tm2c_tx_temp->start_ts = 0;
#endif

  return tm2c_tx_temp;
}

void
tm2c_tx_meta_free(tm2c_tx_t **tm2c_tx) 
{
#if !defined(PGAS)
  write_set_free((*tm2c_tx)->write_set);
#endif
  mem_info_free((*tm2c_tx)->mem_info);
  free((*tm2c_tx));
  *tm2c_tx = NULL;
}
