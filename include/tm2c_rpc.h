/*
 *   File: tmc_rpc.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: structures that define the RPC (messages) of TM2C
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

#ifndef _TM2C_RPC_H_
#define _TM2C_RPC_H_

/*
 * In this header file, we defined the messages that nodes use to communicate.
 * This way, we can include just the data definition in necessary files.
 */

#ifdef	__cplusplus
extern "C" {
#endif

  typedef enum 
    {
      TM2C_RPC_LOAD,		//0
      TM2C_RPC_STORE,		//1
      TM2C_RPC_LOAD_RLS,		//2
      TM2C_RPC_STORE_FINISH,	//3
      TM2C_RPC_RMV_NODE,		//4
      TM2C_RPC_LOAD_NONTX,		//5
      TM2C_RPC_STORE_NONTX,		//6
      TM2C_RPC_STORE_INC,		//7
      TM2C_RPC_STATS,			//8
      TM2C_RPC_UKNOWN			//9
    } TM2C_RPC_REQ_TYPE;

  typedef enum 
    {
      TM2C_RPC_LOAD_RESPONSE,
      TM2C_RPC_STORE_RESPONSE,
      TM2C_RPC_ABORTED,
      TM2C_RPC_LOAD_NONTX_RESPONSE,
      TM2C_RPC_STORE_NONTX_RESPONSE,
      TM2C_RPC_UKNOWN_RESPONSE
    } TM2C_RPC_REPLY_TYPE;


  /* A command to the DSL service */
#if defined(PLATFORM_TILERA)

#  if defined(WHOLLY) || defined(GREEDY) || defined(FAIRCM) || defined(PGAS)
#    if defined(__tilegx__)
#      define TM2C_RPC_REQ_SIZE       24
#      define TM2C_RPC_REQ_SIZE_WORDS 3
#    endif  /* __tilegx__ */
#  else	 /* defined(NOCM) */
#    if  defined( __tilegx__)
#      define TM2C_RPC_REQ_SIZE       16
#      define TM2C_RPC_REQ_SIZE_WORDS 2
#    endif
#  endif

  typedef struct tm2c_rpc_req_struct 
  {
    struct
    {
      uint8_t type;	      /* TM2C_RPC_REQ_TYPE */
      uint8_t nodeId;	      /* we need IDs on networked systems */
      tm_intern_addr_t address; /* addr of the data, internal representation */

#  if defined(PGAS)
      union 
      {
	int64_t response;       /* used in TM2C_RPC_RMV_NODE with PGAS to say persist or not */
	int64_t write_value;
	uint64_t num_words;
      };
#  endif	/* PGAS */
#  if !defined(NOCM) && !defined(BACKOFF_RETRY)
      uint64_t tx_metadata;
#  endif  /* !NOCM */
    };
    /* OPTIONAL based on PGAS and NOCM */
  } TM2C_RPC_REQ;
  

#  if defined(PGAS)
#    define TM2C_RPC_REPLY_SIZE       12
#    define TM2C_RPC_REPLY_SIZE_WORDS 3
#  else
#    define TM2C_RPC_REPLY_SIZE       4
#    define TM2C_RPC_REPLY_SIZE_WORDS 1
#  endif

  typedef struct tm2c_rpc_reply_struct 
  {
    union
    {
      struct
      {
	uint8_t type;
	uint8_t response;
	uint8_t nodeId;
	uint8_t resp_size;
      };
      uint32_t to_word;
    };
  /* tm_intern_addr_t address; /\* address of the data, internal representation *\/ */

#  if defined(PGAS)
  int64_t value;
#  endif	/* PGAS */
} TM2C_RPC_REPLY;


#  define TM2C_RPC_STATS_SIZE       32
#  if defined(__tilepro__)
#    define TM2C_RPC_STATS_SIZE_WORDS 8
#  else  /* __tilegx__ */
#    define TM2C_RPC_STATS_SIZE_WORDS 4
#  endif

//A command to the pub-sub
typedef struct tm2c_rpc_stats_struct 
{
  uint8_t type; //TM2C_RPC_REQ_TYPE
  uint8_t  nodeId;	/* we need IDs on networked systems */

  struct
  {
    union 
    {			
      struct 			/* 12 */
      {
	uint32_t aborts;
	uint32_t commits;
	uint32_t max_retries;
      };
      struct /* 12 */
      {
	uint32_t aborts_war;
	uint32_t aborts_raw;
	uint32_t aborts_waw;
      };
    };

    uint32_t dummy;
    double tx_duration;
  };

} TM2C_RPC_STATS_T;


  /* ____________________________________________________________________________________________________ */
#elif defined(PLATFORM_NIAGARA)
  /* ____________________________________________________________________________________________________ */

#    define TM2C_RPC_REQ_SIZE       32
#    define TM2C_RPC_REQ_SIZE_WORDS 4

  typedef struct tm2c_rpc_req_struct 
  {
    struct
    {
      tm_intern_addr_t address; /* 8 */
      uint8_t type;		/* 8 */
      uint8_t nodeId;		
      union 			/* 8 */
      {
	int64_t response;       
	int64_t write_value;
	uint64_t num_words;
      };
      uint64_t tx_metadata;	/* 8 */
    };
  } TM2C_RPC_REQ;
  

#    define TM2C_RPC_REPLY_SIZE       TM2C_RPC_REQ_SIZE
#    define TM2C_RPC_REPLY_SIZE_WORDS TM2C_RPC_REQ_SIZE_WORDS

  typedef struct tm2c_rpc_reply_struct 
  {
    union
    {
      struct
      {
	uint8_t type;
	uint8_t response;
	uint8_t nodeId;
	uint8_t resp_size;
      };
      uint32_t to_word;
    };
    uint8_t padding1[4];
    int64_t value;
    uint8_t padding2[16];
  } TM2C_RPC_REPLY;


#  define TM2C_RPC_STATS_SIZE       TM2C_RPC_REQ_SIZE
#  define TM2C_RPC_STATS_SIZE_WORDS TM2C_RPC_REQ_SIZE_WORDS

//A command to the pub-sub
typedef struct tm2c_rpc_stats_struct 
{
  double tx_duration;
  uint8_t type; //TM2C_RPC_REQ_TYPE
  uint8_t nodeId;	/* we need IDs on networked systems */
  struct
  {
    union 
    {			
      struct 			/* 12 */
      {
	uint32_t aborts;
	uint32_t commits;
	uint32_t max_retries;
      };
      struct /* 12 */
      {
	uint32_t aborts_war;
	uint32_t aborts_raw;
	uint32_t aborts_waw;
      };
    };

    uint8_t padding[4];
  };
} TM2C_RPC_STATS_T;

//A command to the pub-sub
typedef struct tm2c_rpc_stats_send
{
  union 
  {			
    double tx_duration;
    uint32_t aborts;
    uint32_t commits;
    uint32_t max_retries;
    uint32_t aborts_war;
    uint32_t aborts_raw;
    uint32_t aborts_waw;
  };

  uint8_t type; //TM2C_RPC_REQ_TYPE
  uint8_t nodeId;	/* we need IDs on networked systems */
  uint8_t stats_type;
  uint8_t padding[20];
} TM2C_RPC_STATS_SEND_T;

  /* ____________________________________________________________________________________________________ */
#else  /* !PLATFORM_TILERA && !NIAGARA                                                                    */
  /* ____________________________________________________________________________________________________ */

  typedef struct tm2c_rpc_req_struct 
  {
    int32_t type;	      /* TM2C_RPC_REQ_TYPE */
    nodeid_t nodeId;	      /* we need IDs on networked systems */
    /* 8 */
    tm_intern_addr_t address; /* addr of the data, internal representation */
    /* 8 */
    union 
    {
      int64_t response;       /* used in TM2C_RPC_RMV_NODE with PGAS to say persist or not */
      int64_t write_value;
      uint64_t num_words;
    };
    /* 8 */
    uint64_t tx_metadata;
#  if defined(SCC)
    int32_t dummy[1];		/* tm_inter_add = size_t = size 4 on the SCC */
#  endif
    /* SSMP[.SCC] uses the last word as a flag, hence it should be not used for data */
#  if defined(SSMP)
    uint8_t padding[32];
#  endif
  } TM2C_RPC_REQ;


  /* a reply message */
  typedef struct tm2c_rpc_reply_struct 
  {
    int32_t type; //TM2C_RPC_REPLY_TYPE
    nodeid_t nodeId;
    tm_intern_addr_t address; /* address of the data, internal representation */
    int64_t value;
    int32_t response; //BOOLEAN
    int32_t resp_size;
#  if defined(SCC)
    int32_t dummy[1];		/* tm_inter_add = size_t = size 4 on the SCC */
#  endif
    /* SSMP[.SCC] uses the last word as a flag, hence it should be not used for data */
#  if defined(SSMP)
    uint8_t padding[32];
#  endif
  } TM2C_RPC_REPLY;

//A command to the pub-sub
typedef struct tm2c_rpc_stats_struct 
{
  int32_t type; //TM2C_RPC_REQ_TYPE
  nodeid_t  nodeId;	/* we need IDs on networked systems */
  /* 8 */
  /* stats collecting --------------------*/
  struct
  {
    union 
    {			
      struct 			/* 12 */
      {
	uint32_t aborts;
	uint32_t commits;
	uint32_t max_retries;
      };
      struct /* 12 */
      {
	uint32_t aborts_war;
	uint32_t aborts_raw;
	uint32_t aborts_waw;
      };
    };

    /* SSMP[.SCC] uses the last word as a flag, hence it should be not used for data */
#  if defined(SCC)
    double tx_duration;
    uint32_t mpb_flag;	/* on the SCC, we have CL (MPB) line of 32 bytes */
#  else
    uint32_t dummy;
    double tx_duration;
#  endif      
  };

#  if defined(__x86_64__)
  uint8_t padding[32];
#  endif
} TM2C_RPC_STATS_T;

#endif  /* PLATFORM_TILERA */

#ifdef	__cplusplus
}
#endif

#endif /* _TM2C_RPC_H_ */
