/*
 *   File: tm2c_dsl.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: main functionality for service processes
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
 * File:   tm2c_app.c
 * Author: trigonak
 *
 * Created on March 7, 2011, 4:45 PM
 * working
 */

#include "tm2c_dsl.h"
#include <stdio.h> //TODO: clean the includes
#include <unistd.h>
#include <math.h>

tm2c_ht_t tm2c_ht;

#ifdef PGAS
tm2c_write_set_pgas_t** PGAS_write_sets;
#endif

unsigned long int tm2c_stats_total = 0,
                  tm2c_stats_commits = 0,
                  tm2c_stats_aborts = 0,
                  tm2c_stats_max_retries = 0,
                  tm2c_stats_aborts_war = 0,
                  tm2c_stats_aborts_raw = 0,
                  tm2c_stats_aborts_waw = 0,
                  tm2c_stats_received = 0;
double tm2c_stats_duration = 0;

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
cm_metadata_t* cm_metadata_core;
#endif

#ifdef DEBUG_UTILIZATION
extern unsigned int read_reqs_num;
extern unsigned int write_reqs_num;
int bucket_usages[NUM_OF_BUCKETS];
int bucket_current[NUM_OF_BUCKETS];
int bucket_max[NUM_OF_BUCKETS];
#endif

extern void tm2c_term();

void
tm2c_dsl_init(void)
{
#ifdef PGAS
  PGAS_write_sets = (tm2c_write_set_pgas_t **) malloc(NUM_UES * sizeof (tm2c_write_set_pgas_t *));
  if (PGAS_write_sets == NULL)
    {
      PRINT("malloc PGAS_write_sets == NULL");
      EXIT(-1);
    }

  nodeid_t j;
  for (j = 0; j < NUM_UES; j++)
    {
      if (is_app_core(j))
	{ /*only for non DSL cores*/
	  PGAS_write_sets[j] = write_set_pgas_new();
	  if (PGAS_write_sets[j] == NULL)
	    {
	      PRINT("malloc PGAS_write_sets[i] == NULL");
	      EXIT(-1);
	    }
	}
    }

#endif

  tm2c_ht = tm2c_ht_new();

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
  cm_metadata_core = (cm_metadata_t *) calloc(NUM_UES, sizeof(cm_metadata_t));
  if (cm_metadata_core == NULL)
    {
      PRINT("calloc @ tm2c_dsl_init");
    }
#endif

  sys_dsl_init();

  /* PRINT("[DSL NODE] Initialized pub-sub.."); */
  dsl_service();

  BARRIERW;
  tm2c_term();
    
  term_system();
  EXIT(0);
}

void
tm2c_dsl_print_global_stats() 
{
  tm2c_stats_duration /= NUM_APP_NODES;

  if (tm2c_stats_commits || tm2c_stats_aborts || tm2c_stats_total)
    {
      printf("||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
      printf("TXs Statistics : %02d Nodes, %02d App Nodes|||||||||||||||||||||||\n", NUM_UES, NUM_APP_NODES);
      printf(":: TOTAL -----------------------------------------------------\n");
      printf("T | Avg Duration\t: %.3f s\n", tm2c_stats_duration);
      printf("T | Starts      \t: %lu\n", tm2c_stats_total);
      printf("T | Commits     \t: %lu\n", tm2c_stats_commits);
      printf("T | Aborts      \t: %lu\n", tm2c_stats_aborts);
      printf("T | Max Retries \t: %lu\n", tm2c_stats_max_retries);
      printf("T | Aborts WAR  \t: %lu\n", tm2c_stats_aborts_war);
      printf("T | Aborts RAW  \t: %lu\n", tm2c_stats_aborts_raw);
      printf("T | Aborts WAW  \t: %lu\n", tm2c_stats_aborts_waw);

      tm2c_stats_aborts /= tm2c_stats_duration;
      tm2c_stats_aborts_raw /= tm2c_stats_duration;
      tm2c_stats_aborts_war /= tm2c_stats_duration;
      tm2c_stats_aborts_waw /= tm2c_stats_duration;
      tm2c_stats_commits /= tm2c_stats_duration;
      tm2c_stats_total /= tm2c_stats_duration;

      unsigned long int tm2c_stats_commits_total = tm2c_stats_commits;

      printf(":: PER SECOND TOTAL AVG --------------------------------------\n");
      printf("TA| Starts      \t: %lu\t/s\n", tm2c_stats_total);
      printf("TA| Commits     \t: %lu\t/s\n", tm2c_stats_commits);
      printf("TA| Aborts      \t: %lu\t/s\n", tm2c_stats_aborts);
      printf("TA| Aborts WAR  \t: %lu\t/s\n", tm2c_stats_aborts_war);
      printf("TA| Aborts RAW  \t: %lu\t/s\n", tm2c_stats_aborts_raw);
      printf("TA| Aborts WAW  \t: %lu\t/s\n", tm2c_stats_aborts_waw);

      int tm2c_stats_commits_app = tm2c_stats_commits / NUM_APP_NODES;

      tm2c_stats_aborts /= NUM_UES;
      tm2c_stats_aborts_raw /= NUM_UES;
      tm2c_stats_aborts_war /= NUM_UES;
      tm2c_stats_aborts_waw /= NUM_UES;
      tm2c_stats_commits /= NUM_UES;
      tm2c_stats_total /= NUM_UES;

      double commit_rate = (tm2c_stats_total - tm2c_stats_aborts) / (double) tm2c_stats_total;
      double tx_latency = (1 / (double) tm2c_stats_commits_app) * 1000 * 1000; //micros

      printf(":: PER SECOND PER NODE AVG -----------------------------------\n");
      printf("NA| Starts      \t: %lu\t/s\n", tm2c_stats_total);
      printf("NA| Commits     \t: %lu\t/s\n", tm2c_stats_commits);
      printf("NA| Aborts      \t: %lu\t/s\n", tm2c_stats_aborts);
      printf("NA| Aborts WAR  \t: %lu\t/s\n", tm2c_stats_aborts_war);
      printf("NA| Aborts RAW  \t: %lu\t/s\n", tm2c_stats_aborts_raw);
      printf("NA| Aborts WAW  \t: %lu\t/s\n", tm2c_stats_aborts_waw);
      printf(":: Collect data ----------------------------------------------\n");
      printf("))) %-10lu%-7.2f%-.3f%5d\t(Throughput, Commit Rate, Latency)\n", tm2c_stats_commits_total, commit_rate * 100, tx_latency, NUM_DSL_NODES);
      printf("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");

      fflush(stdout);
    }
}

