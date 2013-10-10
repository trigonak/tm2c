/*
 *   File: sys_tilera.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: implementations of platform specific functions for
 *                Tilera
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

#include "common.h"
#include "tm2c_app.h"
#include "tm2c_dsl.h"
#include "tm2c_malloc.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#if !defined(NOCM) && !defined(BACKOFF_RETRY) 			/* if any other CM (greedy, wholly, faircm) */
int32_t **cm_abort_flags;
int32_t *cm_abort_flag_mine;
#endif /* NOCM */


#define DEBUG_UTILIZATION_OFF
#ifdef  DEBUG_UTILIZATION
unsigned int read_reqs_num = 0, write_reqs_num = 0;
#endif

#define TM_MEM_SIZE      (128 * 1024 * 1024)

nodeid_t TM2C_ID;
nodeid_t TM2C_NUM_NODES;

static TM2C_RPC_REQ* tm2c_rpc_remote;
static TM2C_RPC_REPLY* tm2c_rpc_reply;
DynamicHeader* udn_header; //headers for messaging
/* uint32_t* demux_tags; */
/* uint32_t demux_tag_mine; */
tmc_sync_barrier_t *barrier_apps, *barrier_all, *barrier_dsl; //BARRIERS

void
sys_tm2c_init_system(int* argc, char** argv[])
{
    if (*argc < 2)
    {
      fprintf(stderr, "Not enough parameters (%d)\n", *argc);
      fprintf(stderr, "Call this program as:\n");
      fprintf(stderr, "\t%s -total=TOTAL_NODES ...\n", (*argv)[0]);
      EXIT(1);
    }

  int p = 1;
  int found = 0;
  while (p < *argc)
    {
      if (strncmp("-total=", (*argv)[p], strlen("-total=")) == 0)
	{
	  char* cf = (*argv)[p] + strlen("-total=");
	  NUM_UES = atoi(cf);
	  (*argv)[p] = NULL;
	  found = 1;
	}
      p++;
    }
  if (!found)
    {
      fprintf(stderr, "Did not pass all parameters\n");
      fprintf(stderr, "Call this program as:\n");
      fprintf(stderr, "\t%s -total=TOTAL_NODES ...\n", (*argv)[0]);
      EXIT(1);
    }
  p = 1;
  int cur = 1;
  while (p < *argc)
    {
      if ((*argv)[p] == NULL)
	{
	  p++;
	  continue;
	}
      (*argv)[cur] = (*argv)[p];
      cur++;
      p++;
    }
  *argc = *argc - (p-cur);

  /* DO NOT allocate the shared memory if you have PGAS mem model */
#ifndef PGAS 
  tmc_alloc_t alloc = TMC_ALLOC_INIT;
  tmc_alloc_set_shared(&alloc);
#  ifdef DISABLE_CC
  tmc_alloc_set_home(&alloc, MAP_CACHE_NO_LOCAL);
#  else
  tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_HASH);
#  endif
  uint32_t* data = tmc_alloc_map(&alloc, TM_MEM_SIZE);
  if (data == NULL)
    {
      tmc_task_die("Failed to allocate memory.");
    }

  tm2c_shmalloc_set((void*) data);
#endif

  //initialize shared memory
  tmc_cmem_init(0);

  cpu_set_t cpus;
  if (tmc_cpus_get_my_affinity(&cpus) != 0)
    tmc_task_die("Failure in 'tmc_cpus_get_my_affinity()'.");

  // Reserve the UDN rectangle that surrounds our cpus.
  if (tmc_udn_init(&cpus) < 0)
    tmc_task_die("Failure in 'tmc_udn_init(0)'.");
  uint32_t i, tot = 0;
  for (i = 0; i < NUM_UES; i++)
    {
      if (!is_app_core(i))
	{
	  tot++;
	}
    }
  NUM_DSL_NODES = tot;
  NUM_APP_NODES = NUM_UES - tot;

  barrier_apps = (tmc_sync_barrier_t *) tmc_cmem_calloc(1, sizeof (tmc_sync_barrier_t));
  barrier_all = (tmc_sync_barrier_t *) tmc_cmem_calloc(1, sizeof (tmc_sync_barrier_t));
  barrier_dsl = (tmc_sync_barrier_t *) tmc_cmem_calloc(1, sizeof (tmc_sync_barrier_t));
  if (barrier_all == NULL || barrier_apps == NULL || barrier_dsl == NULL)
    {
      tmc_task_die("Failure in allocating mem for barriers");
    }
  tmc_sync_barrier_init(barrier_all, NUM_UES);
  tmc_sync_barrier_init(barrier_apps, NUM_APP_NODES);
  tmc_sync_barrier_init(barrier_dsl, NUM_DSL_NODES);

  if (tmc_cpus_get_my_affinity(&cpus) != 0)
    {
      tmc_task_die("Failure in 'tmc_cpus_get_my_affinity()'.");
    }
  if (tmc_cpus_count(&cpus) < NUM_UES)
    {
      tmc_task_die("Insufficient cpus (%d < %d).", tmc_cpus_count(&cpus), NUM_UES);
    }

  int watch_forked_children = tmc_task_watch_forked_children(1);

  TM2C_ID = 0;
  PRINTD("will create %d more procs", NUM_UES - 1);

  int rank;
  for (rank = 1; rank < NUM_UES; rank++)
    {
      pid_t child = fork();
      if (child < 0)
	tmc_task_die("Failure in 'fork()'.");
      if (child == 0)
	goto done;
    }
  rank = 0;

  (void) tmc_task_watch_forked_children(watch_forked_children);

 done:

  TM2C_ID = rank;

  if (tmc_cpus_set_my_cpu(tmc_cpus_find_nth_cpu(&cpus, rank)) < 0)
    {
      tmc_task_die("Failure in 'tmc_cpus_set_my_cpu()'.");
    }

  if (rank != tmc_cpus_get_my_cpu())
    {
      PRINT("******* i am not CPU %d", tmc_cpus_get_my_cpu());
    }

  // Now that we're bound to a core, attach to our UDN rectangle.
  if (tmc_udn_activate() < 0)
    {
      tmc_task_die("Failure in 'tmc_udn_activate()'.");
    }

  udn_header = (DynamicHeader *) malloc(NUM_UES * sizeof (DynamicHeader));
  if (udn_header == NULL)
    {
      tmc_task_die("Failure in allocating dynamic headers");
    }

  int r;
  for (r = 0; r < NUM_UES; r++)
    {
      int _cpu = tmc_cpus_find_nth_cpu(&cpus, r);
      DynamicHeader header = tmc_udn_header_from_cpu(_cpu);
      udn_header[r] = header;
    }

}

void
term_system()
{
  if (tmc_udn_close() != 0)
    {
      tmc_task_die("Failure in 'tmc_udn_close()'.");
    }
}

void tm2c_init_barrier()
{
  //noop
}


void*
sys_shmalloc(size_t size)
{
#ifdef PGAS
  return pgas_app_alloc(size);
#else
  return (void*) tm2c_shmalloc(size);
#endif
}

void
sys_shfree(void* ptr)
{
#ifdef PGAS
  pgas_app_free((void*) ptr);
#else
  tm2c_shfree(ptr);
#endif
}

/* static TM2C_CONFLICT_T tm2c_rpc_resp; */

static TM2C_RPC_REQ *psc;

void
sys_tm2c_init() 
{
}

void
sys_app_init(void)
{

#if defined(PGAS)
  pgas_app_init();
#endif

#if !defined(NOCM) && !defined(BACKOFF_RETRY) 			/* if any other CM (greedy, wholly, faircm) */
  cm_abort_flag_mine = cm_init(NODE_ID());
  *cm_abort_flag_mine = NO_CONFLICT;
#endif
  BARRIERW;

  /* demux_tags = (uint32_t*) malloc(TOTAL_NODES() * sizeof(uint32_t)); */
  /* assert(demux_tags != NULL); */

  /* nodeid_t n; */
  /* for (n = 0; n < TOTAL_NODES(); n++) */
  /*   { */
  /* if (is_dsl_core(n)) */
  /* 	{ */
  /* 	  uint32_t id_seq = dsl_id_seq(n); */

  /* switch (id_seq % 4) */
  /*   { */
  /*   case 0: */
  /*     demux_tags[n] = UDN0_DEMUX_TAG; */
  /*     break; */
  /*   case 1: */
  /*     demux_tags[n] = UDN1_DEMUX_TAG; */
  /*     break; */
  /*   case 2: */
  /*     demux_tags[n] = UDN2_DEMUX_TAG; */
  /*     break; */
  /*   default: 			/\* 3 *\/ */
  /*     demux_tags[n] = UDN3_DEMUX_TAG; */
  /*     break; */
  /*   } */
  /* } */
  /* } */

  BARRIERW;
}

void
sys_dsl_init(void)
{
#if defined(PGAS)
  pgas_dsl_init();
#endif

  tm2c_rpc_remote = (TM2C_RPC_REQ*) malloc(sizeof (TM2C_RPC_REQ)); //TODO: free at finalize + check for null
  psc = (TM2C_RPC_REQ*) malloc(sizeof (TM2C_RPC_REQ)); //TODO: free at finalize + check for null
  tm2c_rpc_reply = (TM2C_RPC_REPLY*) malloc(sizeof(TM2C_RPC_REPLY));
  assert(tm2c_rpc_remote != NULL && psc != NULL && tm2c_rpc_reply != NULL);

  /* nodeid_t id_seq = dsl_id_seq(NODE_ID()); */
  /* switch (id_seq % 4) */
  /*   { */
  /*   case 0: */
  /*     demux_tag_mine = UDN0_DEMUX_TAG; */
  /*     break; */
  /*   case 1: */
  /*     demux_tag_mine = UDN1_DEMUX_TAG; */
  /*     break; */
  /*   case 2: */
  /*     demux_tag_mine = UDN2_DEMUX_TAG; */
  /*     break; */
  /*   default: 			/\* 3 *\/ */
  /*     demux_tag_mine = UDN3_DEMUX_TAG; */
  /*     break; */
  /*   } */

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
  cm_abort_flags = (int32_t **) malloc(TOTAL_NODES() * sizeof(int32_t *));
  assert(cm_abort_flags != NULL);

  BARRIERW;

  uint32_t i;
  for (i = 0; i < TOTAL_NODES(); i++) 
    {
      //TODO: make it open only for app nodes
      if (is_app_core(i))
	{
	  cm_abort_flags[i] = cm_init(i);    
	}
    }
#else
  BARRIERW;
#endif

  BARRIERW;
}

void
sys_dsl_term(void)
{
#if defined(PGAS)
  pgas_dsl_term();
#else  /* PGAS */
  tm2c_shmalloc_term();
#endif	/* PGAS */


#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if real cm: wholly, greedy, faircm */
  assert(cm_abort_flags != NULL);

  uint32_t i;
  for (i = 0; i < TOTAL_NODES(); i++) 
    {
      if (is_app_core(i))
	{
	  cm_term(i);    
	}
    }

  free(cm_abort_flags);
#endif

  BARRIERW;
}

void
sys_app_term(void)
{
#if defined(PGAS)
  pgas_app_term();
#else  /* PGAS */
  tm2c_shmalloc_term();
#endif /* PGAS */

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if real cm: wholly, greedy, faircm */
  cm_term(NODE_ID());
#  if defined(GREEDY) && defined(GREEDY_GLOBAL_TS)
  /* cm_greedy_global_ts_term(); */
#  endif

#endif

  BARRIERW;
}
// If value == NULL, we just return the address.
// Otherwise, we return the value.

INLINED void 
sys_tm2c_rpc_req_reply(nodeid_t sender,
		     TM2C_RPC_REPLY_TYPE command,
		     tm_addr_t address,
		     uint32_t* value,
		     TM2C_CONFLICT_T response)
{
  tm2c_rpc_reply->type = command;
  tm2c_rpc_reply->response = response;

  PRINTD("sys_tm2c_rpc_req_reply: src=%u target=%d", tm2c_rpc_reply->nodeId, sender);
#ifdef PGAS
  tm2c_rpc_reply->value = value;
#endif

  tmc_udn_send_buffer(udn_header[sender], UDN0_DEMUX_TAG, tm2c_rpc_reply, TM2C_RPC_REPLY_SIZE_WORDS);

  /* tmc_udn_send_buffer(udn_header[sender], demux_tag_mine, &reply, TM2C_RPC_REPLY_WORDS); */
}


void 
dsl_service()
{
  nodeid_t sender;
  /* uint32_t* cmd = (uint32_t*) tm2c_rpc_remote; */

  PF_MSG(5, "servicing a request");
  while (1) 
    {
      /* PF_START(5); */
      tmc_udn0_receive_buffer(tm2c_rpc_remote, TM2C_RPC_REQ_SIZE_WORDS);

      sender = tm2c_rpc_remote->nodeId;
      
#if defined(WHOLLY) || defined(FAIRCM)
      cm_metadata_core[sender].timestamp = (ticks) tm2c_rpc_remote->tx_metadata;
#elif defined(GREEDY)
      if (cm_metadata_core[sender].timestamp == 0)
	{
#  ifdef GREEDY_GLOBAL_TS
	  cm_metadata_core[sender].timestamp = (ticks) tm2c_rpc_remote->tx_metadata;
#  else
	  cm_metadata_core[sender].timestamp = getticks() - (ticks) tm2c_rpc_remote->tx_metadata;
#  endif
	}
#endif
      /* PRINT("CMD from %02d | type: %d | addr: %u", sender, tm2c_rpc_remote->type, tm2c_rpc_remote->address); */

      switch (tm2c_rpc_remote->type) 
	{
	case TM2C_RPC_LOAD:
	  {

	    TM2C_CONFLICT_T conflict = try_load(sender, tm2c_rpc_remote->address);
#ifdef PGAS
	    uint64_t val;
	    if (tm2c_rpc_remote->num_words == 1)
	      {
		val = pgas_dsl_read32(tm2c_rpc_remote->address);
	      }
	    else
	      {
		val = pgas_dsl_read(tm2c_rpc_remote->address);
	      }

	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_LOAD_RESPONSE,
				 (tm_addr_t) tm2c_rpc_remote->address, val, conflict);
#else  /* !PGAS */
	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_LOAD_RESPONSE, (tm_addr_t) tm2c_rpc_remote->address, 
				 0, conflict);
#endif	/* PGAS */
	    if (conflict != NO_CONFLICT)
	      {
		tm2c_ht_delete_node(tm2c_ht, sender);
#if defined(GREEDY)
		cm_metadata_core[sender].timestamp = 0;
#endif
#ifdef PGAS
		write_set_pgas_empty(PGAS_write_sets[sender]);
#endif	/* PGAS */
	      }

	    break;
	  }
	case TM2C_RPC_STORE:
	  {
	    TM2C_CONFLICT_T conflict = try_store(sender, tm2c_rpc_remote->address);
	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_STORE_RESPONSE, 
				 (tm_addr_t) tm2c_rpc_remote->address, 0, conflict);

	    if (conflict != NO_CONFLICT)
	      {
		tm2c_ht_delete_node(tm2c_ht, sender);

#if defined(GREEDY)
		cm_metadata_core[sender].timestamp = 0;
#endif
#ifdef PGAS
		write_set_pgas_empty(PGAS_write_sets[sender]);
#endif	/* PGAS */
	      }
#ifdef PGAS
	    else		/* NO_CONFLICT */
	      {
		write_set_pgas_insert(PGAS_write_sets[sender], tm2c_rpc_remote->write_value, tm2c_rpc_remote->address);
	      }
#endif	/* PGAS */
	    break;
	  }
#ifdef PGAS
	case TM2C_RPC_STORE_INC:
	  {
	    TM2C_CONFLICT_T conflict = try_store(sender, tm2c_rpc_remote->address);

	    if (conflict == NO_CONFLICT) 
	      {
		int64_t val = pgas_dsl_read(tm2c_rpc_remote->address) + tm2c_rpc_remote->write_value;
		write_set_pgas_insert(PGAS_write_sets[sender], val, tm2c_rpc_remote->address);
	      }

	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_STORE_RESPONSE,
				 (tm_addr_t) tm2c_rpc_remote->address, 0, conflict);

	    if (conflict != NO_CONFLICT)
	      {
		tm2c_ht_delete_node(tm2c_ht, sender);
		write_set_pgas_empty(PGAS_write_sets[sender]);
#if defined(GREEDY)
		cm_metadata_core[sender].timestamp = 0;
#endif
	      }

	    break;
	  }
	case TM2C_RPC_LOAD_NONTX:
	  {
	    int64_t val;
	    if (tm2c_rpc_remote->num_words == 1)
	      {
		val = (int64_t) pgas_dsl_read32(tm2c_rpc_remote->address);
	      }
	    else
	      {
		val = pgas_dsl_read(tm2c_rpc_remote->address);
	      }

	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_LOAD_NONTX_RESPONSE,
				 (tm_addr_t) tm2c_rpc_remote->address,
				 val,
				 NO_CONFLICT);
		
	    break;
	  }
	case TM2C_RPC_STORE_NONTX:
	  {
	    pgas_dsl_write(tm2c_rpc_remote->address, tm2c_rpc_remote->write_value);
	    break;
	  }
#endif
	case TM2C_RPC_RMV_NODE:
	  {
#ifdef PGAS
	    if (tm2c_rpc_remote->response == NO_CONFLICT) 
	      {
		write_set_pgas_persist(PGAS_write_sets[sender]);
	      }
	    write_set_pgas_empty(PGAS_write_sets[sender]);
#endif
	    tm2c_ht_delete_node(tm2c_ht, sender);

#if defined(GREEDY)
	    cm_metadata_core[sender].timestamp = 0;
#endif
	    break;
	  }
	case TM2C_RPC_LOAD_RLS:
	  tm2c_ht_delete(tm2c_ht, sender, tm2c_rpc_remote->address, READ);
	  break;
	case TM2C_RPC_STORE_FINISH:
	  tm2c_ht_delete(tm2c_ht, sender, tm2c_rpc_remote->address, WRITE);
	  break;
	case TM2C_RPC_STATS:
	  {
	    uint32_t w;
	    int left = TM2C_RPC_STATS_SIZE_WORDS - TM2C_RPC_REQ_SIZE_WORDS;
#if defined(__tilepro__)
	    uint32_t collect[TM2C_RPC_STATS_SIZE_WORDS - TM2C_RPC_REQ_SIZE_WORDS];
#else  /* __tilegx__ */
	    uint64_t collect[TM2C_RPC_STATS_SIZE_WORDS - TM2C_RPC_REQ_SIZE_WORDS];
#endif	/* __tilepro__ */

	    for (w = 0; w < left; w++)
	      {
		collect[w] = tmc_udn0_receive();
	      }

	    TM2C_RPC_STATS_T tm2c_rpc_stats;
	    memcpy(&tm2c_rpc_stats, tm2c_rpc_remote, TM2C_RPC_REQ_SIZE);
	    if (left > 0)
	      {
		void* tm2c_rpc_stats_mid = ((void*) &tm2c_rpc_stats) + TM2C_RPC_REQ_SIZE;
		memcpy(tm2c_rpc_stats_mid, collect, TM2C_RPC_STATS_SIZE - TM2C_RPC_REQ_SIZE);
	      }
	    
	    TM2C_RPC_STATS_T* tm2c_rpc_rem_stats = &tm2c_rpc_stats;

	    if (tm2c_rpc_rem_stats->tx_duration) 
	      {
		tm2c_stats_aborts += tm2c_rpc_rem_stats->aborts;
		tm2c_stats_commits += tm2c_rpc_rem_stats->commits;
		tm2c_stats_duration += tm2c_rpc_rem_stats->tx_duration;
		tm2c_stats_max_retries = tm2c_stats_max_retries < tm2c_rpc_rem_stats->max_retries ? tm2c_rpc_rem_stats->max_retries : tm2c_stats_max_retries;
		tm2c_stats_total += tm2c_rpc_rem_stats->commits + tm2c_rpc_rem_stats->aborts;
	      }
	    else 
	      {
		tm2c_stats_aborts_raw += tm2c_rpc_rem_stats->aborts_raw;
		tm2c_stats_aborts_war += tm2c_rpc_rem_stats->aborts_war;
		tm2c_stats_aborts_waw += tm2c_rpc_rem_stats->aborts_waw;
	      }

	    if (++tm2c_stats_received >= 2*NUM_APP_NODES) 
	      {
		uint32_t n;
		for (n = 0; n < TOTAL_NODES(); n++)
		  {
		    BARRIER_DSL;
		    if (n == NODE_ID())
		      {
#if defined(USE_HASHTABLE_SSHT)
			ssht_stats_print(tm2c_ht, SSHT_DBG_UTILIZATION_DTL);
#endif
		      }
		    BARRIER_DSL;
		  }

		BARRIER_DSL;
		if (NODE_ID() == min_dsl_id()) 
		  {
		    tm2c_dsl_print_global_stats();
		  }
		return;
	      }
	    break;
	  }
	default:
	  {
	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_UKNOWN_RESPONSE, NULL, 0, NO_CONFLICT);
	  }
	}
    }

}




/*
 * Seeding the rand()
 */
void
srand_core() 
{
  double timed_ = wtime();
  unsigned int timeprfx_ = (unsigned int) timed_;
  unsigned int time_ = (unsigned int) ((timed_ - timeprfx_) * 1000000);
  srand(time_ + (13 * (NODE_ID() + 1)));
}


inline void
wait_cycles(uint64_t ncycles)
{
  if (ncycles < 10000)
    {
      uint32_t cy = (((uint32_t) ncycles) / 8);
      while(cy--)
	{
	  cycle_relax();
	}
    }
  else
    {
      ticks __end = getticks() + ncycles;
      while (getticks() < __end);
    }
}

void
udelay(uint64_t micros) 
{
  ticks in_cycles = REF_SPEED_GHZ * 1000 * micros;
  wait_cycles(in_cycles);
}

void 
ndelay(uint64_t nanos)
{
  ticks in_cycles = REF_SPEED_GHZ * nanos;
  wait_cycles(in_cycles);
}


#if !defined(NOCM) && !defined(BACKOFF_RETRY)	/* if any other CM (greedy, wholly, faircm) */
int32_t*
cm_init(nodeid_t node)
{
  char keyF[50];
  sprintf(keyF,"/cm_abort_flag%03d", node);

  size_t cache_line = 64;

  int abrtfd = shm_open(keyF, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
  if (abrtfd<0)
    {
      if (errno != EEXIST)
	{
	  perror("In shm_open");
	  exit(1);
	}

      //this time it is ok if it already exists                                                    
      abrtfd = shm_open(keyF, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
      if (abrtfd<0)
	{
	  perror("In shm_open");
	  exit(1);
	}
    }
  else
    {
      //only if it is just created                                                                 
      if(ftruncate(abrtfd, cache_line))
	{
	  printf("ftruncate");
	}
    }

  int32_t *tmp = (int32_t *) mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_SHARED, abrtfd, 0);
  assert(tmp != NULL);
   
  return tmp;
}

void
cm_term(nodeid_t node)
{
  char keyF[50];
  sprintf(keyF,"/cm_abort_flag%03d", node);
  shm_unlink(keyF);
}

#endif	/* NOCM */
