/*
 *   File: sys_scc.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: implementations of platform specific functions for Intel SCC
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
#include "RCCE.h"
#include "tm2c_app.h"
#include "tm2c_dsl.h"
#include "ssmp.h"

#ifdef PGAS
/*
 * Under PGAS we're using pgas_app allocator, to have fake allocations, that
 * is a very simplified allocator
 */
#  include "pgas_app.h"
#endif

nodeid_t TM2C_ID;
nodeid_t TM2C_NUM_NODES;

#if !defined(NOCM) && !defined(BACKOFF_RETRY) /* if any other CM (greedy, wholly, faircm) */
ssmp_mpb_line_t *abort_reason_mine;
ssmp_mpb_line_t **abort_reasons;
volatile ssmp_mpb_line_t *persisting_mine;
volatile ssmp_mpb_line_t **persisting;
t_vcharp *cm_abort_flags;
t_vcharp cm_abort_flag_mine;
#endif /* CM_H */

INLINED void sys_tm2c_rpc_req_reply(nodeid_t sender,
                    TM2C_RPC_REPLY_TYPE command,
                    tm_addr_t address,
                    int64_t value,
                    TM2C_CONFLICT_T response);

void 
sys_tm2c_init_system(int* argc, char** argv[])
{
  RCCE_init(argc, argv);

  TM2C_ID = RCCE_ue();
  TM2C_NUM_NODES = RCCE_num_ues();

  ssmp_init(RCCE_num_ues(), RCCE_ue());
}

void
term_system()
{
  ssmp_term();
}

sys_t_vcharp
sys_shmalloc(size_t size)
{
#ifdef PGAS
  return pgas_app_alloc(size);
#else
  return RCCE_shmalloc(size);
#endif
}

void
sys_shfree(sys_t_vcharp ptr)
{
#ifdef PGAS
  pgas_app_free((void*) ptr);
#else
  RCCE_shfree(ptr);
#endif
}

static TM2C_CONFLICT_T tm2c_rpc_resp; //TODO: make it more sophisticated
static TM2C_RPC_REQ *tm2c_rpc_req, *tm2c_rpc_remote, *psc;

#ifndef PGAS
/*
 * Pointer to the minimum address we get from the iRCCE_shmalloc
 * Used for offsets, set in tm2c_init
 * Not used with PGAS, as there we rely on fakemem_malloc
 */
tm_addr_t shmem_start_address;
#endif

void
sys_tm2c_init()
{
  
#ifndef PGAS
    if (shmem_start_address == NULL) {
        char *start = (char *)RCCE_shmalloc(sizeof (char));
        if (start == NULL) {
            PRINTD("shmalloc shmem_init_start_address");
        }
        shmem_start_address = (tm_addr_t)start;
        RCCE_shfree((volatile unsigned char*) start);
    }
#endif

}

void
sys_app_init(void)
{

#ifndef NOCM 			/* if any other CM (greedy, wholly, faircm) */
  abort_reason_mine = (ssmp_mpb_line_t *) ssmp_mpb_alloc(NODE_ID(), sizeof(ssmp_mpb_line_t));
  persisting_mine = (ssmp_mpb_line_t *) ssmp_mpb_alloc(NODE_ID(), sizeof(ssmp_mpb_line_t));
  mpb_write_line(persisting_mine, 0);
  cm_abort_flag_mine = virtual_lockaddress[NODE_ID()];
#endif /* CM_H */

#if defined(PGAS)
  pgas_app_init();
#endif  /* PGAS */

  BARRIERW;
}

void
sys_dsl_init(void)
{
  tm2c_rpc_req = (TM2C_RPC_REQ *) malloc(sizeof (TM2C_RPC_REQ)); //TODO: free at finalize
  tm2c_rpc_remote = (TM2C_RPC_REQ *) malloc(sizeof (TM2C_RPC_REQ)); //TODO: free at finalize
  psc = (TM2C_RPC_REQ *) malloc(sizeof (TM2C_RPC_REQ)); //TODO: free at finalize

  if (tm2c_rpc_req == NULL || tm2c_rpc_remote == NULL || psc == NULL) {
    PRINTD("malloc tm2c_rpc_req == NULL || tm2c_rpc_remote == NULL || psc == NULL");
  }

#ifndef NOCM 			/* if any other CM (greedy, wholly, faircm) */
  abort_reasons = (ssmp_mpb_line_t **) malloc(TOTAL_NODES() * sizeof(ssmp_mpb_line_t *));
  persisting = (volatile ssmp_mpb_line_t **) malloc(TOTAL_NODES() * sizeof(ssmp_mpb_line_t *));
  assert(abort_reasons != NULL && persisting != NULL);
  uint32_t n;
  for (n = 0; n < TOTAL_NODES(); n++)
    {
      if (is_app_core(n)) 
	{
	  abort_reasons[n] = (ssmp_mpb_line_t *) ssmp_mpb_alloc(n, sizeof(ssmp_mpb_line_t));
	  persisting[n] = (ssmp_mpb_line_t *) ssmp_mpb_alloc(n, sizeof(ssmp_mpb_line_t));
	}
      else 
	{
	  abort_reasons[n] = NULL;
	  persisting[n] = NULL;
	}
  }
  cm_abort_flags = virtual_lockaddress;
#endif /* CM_H */

#if defined(PGAS)
  pgas_dsl_init();
#endif	/* PGAS */

  BARRIERW;
}

void
sys_dsl_term(void)
{
	// noop
}

void
sys_app_term(void)
{
	// noop
}

// If value == NULL, we just return the address.
// Otherwise, we return the value.
INLINED void
sys_tm2c_rpc_req_reply(nodeid_t sender, TM2C_RPC_REPLY_TYPE cmd, tm_addr_t addr, int64_t value, TM2C_CONFLICT_T response)
{
  TM2C_RPC_REPLY reply;
  reply.type = cmd;
  reply.response = response;

  PRINTD("sys_tm2c_rpc_req_reply: src=%u target=%d", reply.nodeId, sender);
#ifdef PGAS
  reply.value = value;
#endif

  ssmp_msg_t *msg = (ssmp_msg_t *) &reply;
  ssmp_send(sender, msg);
}

typedef struct
{
  union
  {
    int64_t lli;
    int32_t i[2];
  };
} convert_t;

void
dsl_service()
{
  nodeid_t sender, last_recv_from = 0;
  TM2C_RPC_REQ_TYPE command;

  ssmp_msg_t *msg;
  msg = (ssmp_msg_t *) malloc(sizeof(ssmp_msg_t));

  ssmp_color_buf_t *cbuf;
  cbuf = (ssmp_color_buf_t *) malloc(sizeof(ssmp_color_buf_t));
  assert(msg != NULL && cbuf != NULL);

  ssmp_color_buf_init(cbuf, is_app_core);

  while (1)
    {

      last_recv_from = ssmp_recv_color_start(cbuf, msg, last_recv_from) + 1;
      sender = msg->sender;
    
      tm2c_rpc_remote = (TM2C_RPC_REQ *) msg;

#if defined(WHOLLY) || defined(FAIRCM)
      cm_metadata_core[sender].timestamp = (ticks) tm2c_rpc_remote->tx_metadata;
#elif defined(GREEDY)
      if (cm_metadata_core[sender].timestamp == 0)
	{
	  cm_metadata_core[sender].timestamp = getticks() - (ticks) tm2c_rpc_remote->tx_metadata;
	}
#endif

      switch (tm2c_rpc_remote->type)
	{
	case TM2C_RPC_LOAD:
	  {
	    TM2C_CONFLICT_T conflict = try_load(sender, tm2c_rpc_remote->address);
	    /* PF_STOP(11); */
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
	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_LOAD_RESPONSE, 
				 (tm_addr_t) tm2c_rpc_remote->address, 
				 0,
				 conflict);
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
#ifdef PGAS

	    if (conflict == NO_CONFLICT) 
	      {
		write_set_pgas_insert(PGAS_write_sets[sender], tm2c_rpc_remote->write_value, tm2c_rpc_remote->address);
	      }
#endif	/* PGAS */

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

	    break;
	  }
#ifdef PGAS
	case TM2C_RPC_STORE_INC:
	  {
	    TM2C_CONFLICT_T conflict = try_store(sender, tm2c_rpc_remote->address);

	    if (conflict == NO_CONFLICT) 
	      {
		int64_t val = pgas_dsl_read(tm2c_rpc_remote->address) + tm2c_rpc_remote->write_value;

		/* PRINT("TM2C_RPC_STORE_INC from %2d for %3d, off: %lld, old: %3lld, new: %lld", sender,  */
		/*       tm2c_rpc_remote->address, tm2c_rpc_remote->write_value, */
		/*       pgas_dsl_read(tm2c_rpc_remote->address), val); */

		write_set_pgas_insert(PGAS_write_sets[sender], val, tm2c_rpc_remote->address);
	      }

	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_STORE_RESPONSE,
				 (tm_addr_t) tm2c_rpc_remote->address,
				 0,
				 conflict);

	    if (conflict != NO_CONFLICT)
	      {
		tm2c_ht_delete_node(tm2c_ht, sender);
		write_set_pgas_empty(PGAS_write_sets[sender]);
		/* PRINT("conflict on %d (aborting %d)", tm2c_rpc_remote->address, sender); */
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
	    TM2C_RPC_STATS_T* tm2c_rpc_rem_stats = (TM2C_RPC_STATS_T*) tm2c_rpc_remote;

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
			ssht_stats_print(tm2c_ht, SSHT_DBG_UTILIZATION_DTL);
		      }
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
	    sys_tm2c_rpc_req_reply(sender, TM2C_RPC_UKNOWN_RESPONSE,
				 NULL,
				 NULL,
				 NO_CONFLICT);
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
    double timed_ = RCCE_wtime();
    unsigned int timeprfx_ = (unsigned int) timed_;
    unsigned int time_ = (unsigned int) ((timed_ - timeprfx_) * 1000000);
    srand(time_ + (13 * (RCCE_ue() + 1)));
}


INLINED void
wait_cycles(uint64_t ncycles)
{
  if (ncycles < 24)
    {
      return;
    }
  else if (ncycles < 256) 
    {
      volatile int64_t _ncycles = ncycles;
      _ncycles >>= 3;
      _ncycles -= 3;
      while (_ncycles-- > 0)
	{
	  asm("nop");
	}
    }
  else
    {
      ticks _target = getticks() + ncycles - 50;
      while (getticks() < _target) ;
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

void
tm2c_init_barrier()
{
  ssmp_barrier_init(1, 0, is_app_core);
  ssmp_barrier_init(3, 0, is_dsl_core);

  BARRIERW;
}
