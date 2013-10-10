/*
 *   File: mr.c
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: a simple MapReduc-like example using TM2C
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

#include "tm2c.h"
#include <getopt.h>
#include <stdio.h>

#define PRELOAD_MEMORY

#if defined(SCC)
#  define FILES_FOLDER "/shared/trigonak/"
#else 
#  define FILES_FOLDER "/home/trigonak/code/tm2c/"
#endif

/*
 * Returns the offset of a latin char from a (or A for caps) and the int 26 for
 * non letters. Example: input: b, output: 1
 */
inline int
char_offset(char c)
{
  int a = c - 'a';
  int A = c - 'A';
  if (a < 26 && a >= 0)
    {
      return a;
    }
  else if (A < 26 && A >= 0)
    {
      return A;
    }
  else
    {
      return 26;
    }
}

void map_reduce(FILE *fp, int *chunk_index, int *stats, int* chunks_per_core, char* file_shmem, size_t file_size);
void map_reduce_seq(FILE *fp, int *chunk_index, int *stats, char* file_shmem, size_t file_size);


#define XSTR(s)                 STR(s)
#define STR(s)                  #s

#define DEFAULT_CHUNK_SIZE      4096
#define DEFAULT_FILENAME        "testname"

int chunk_size = DEFAULT_CHUNK_SIZE;
int stats_local[27] = {};
char *filename = DEFAULT_FILENAME;
int sequential = 0;
int local_data = 0;
int abs_path = 0;
char* file_shmem;

int
main(int argc, char** argv)
{
  /* file_shmem = (char*) numa_alloc_interleaved((100LL * 1024 * 1024 * 1024)); */
  /* file_shmem = (char*) numa_alloc_onnode((100LL * 1024 * 1024 * 1024), 7); */
  /* file_shmem = (char*) numa_alloc((100LL * 1024 * 1024 * 1024)); */
  /* file_shmem = (char*) malloc((100LL * 1024 * 1024 * 1024)); */
  /* numa_interleave_memory(file_shmem, (100LL * 1024 * 1024 * 1024), numa_all_nodes_ptr); */
  TM2C_INIT;

  struct option long_options[] =
    {
      // These options don't set a flag
      {"help", no_argument, NULL, 'h'},
      {"filename", required_argument, NULL, 'f'},
      {"chunk", required_argument, NULL, 'c'},
      {"seq", no_argument, NULL, 's'},
      {"local", no_argument, NULL, 'l'},
      {NULL, 0, NULL, 0}
    };

  int i;
  int c;
  while (1)
    {
      i = 0;
      c = getopt_long(argc, argv, "haf:c:sl:", long_options, &i);

      if (c == -1)
	break;

      if (c == 0 && long_options[i].flag == 0)
	c = long_options[i].val;

      switch (c) {
      case 0:
	/* Flag is automatically set */
	break;
      case 'h':
	ONCE
	  {
	    printf("MapReduce -- A transactional MapReduce application\n"
		   "\n"
		   "Usage:\n"
		   "  mr [options...]\n"
		   "\n"
		   "Options:\n"
		   "  -h, --help\n"
		   "        Print this message\n"
		   "  -f, --filename <string>\n"
		   "        File to be loaded and processed\n"
		   "  -a, --absolute\n"
		   "        Use absolute path to the file\n"
		   "  -c, --chunk <int>\n"
		   "        Chuck size (default=" XSTR(DEFAULT_CHUNK_SIZE) ")\n"
		   "  -s, --seq <int>\n"
		   "        Sequential execution\n"
		   "  -l, --local <int>\n"
		   "        load data locally, not from the file (size in MB)\n"
		   );
	    FLUSH;
	  }
	TM_EXIT(0);
      case 'f':
	filename = optarg;
	break;
      case 'c':
	chunk_size = atoi(optarg);
	break;
      case 's':
	sequential = 1;
	break;
      case 'a':
	abs_path = 1;
	break;
      case 'l':
	local_data = atoi(optarg);
	break;
      case '?':
	ONCE
	  {
	    PRINT("Use -h or --help for help\n");
	  }

	TM_EXIT(0);
      default:
	TM_EXIT(1);
      }
    }

  FILE *fp;
  char fn[200];
  if (!abs_path)
    {
      strcpy(fn, FILES_FOLDER);
    }
  else
    {
      strcpy(fn, "");
    }

  strcat(fn, filename);

  fp = fopen(fn, "r");
  if (fp == NULL)
    {
      PRINT("Could not open file %s\n", fn);
      TM_EXIT(1);
    }

  int *chunk_index = (int *) sys_shmalloc(sizeof (int));
  int *stats = (int *) sys_shmalloc(sizeof (int) * 27);
  int *chunks_per_core = (int *) sys_shmalloc(sizeof (int) * TOTAL_NODES());
  if (chunk_index == NULL || stats == NULL || chunks_per_core == NULL)
    {
      PRINT("sys_shmalloc memory @ main");
      TM_EXIT(1);
    }

  size_t file_size;
  if (local_data == 0)
    {
      fseek(fp, 0L, SEEK_END);
      file_size = ftell(fp);
    }
  else
    {
      file_size = local_data * (1024 * 1024LL); /* MB */
    }

  size_t chunk_num = file_size / chunk_size;

  file_shmem = (char*) sys_shmalloc(file_size + 1);
  /* file_shmem = (char*) malloc(file_size + 1); */
  if (file_shmem == NULL)
    {
      PRINT("sys_shmalloc memory @ main for file_shmem");
      TM_EXIT(1);
    }

  /* ONCE */
  /*   { */
  /*     numa_interleave_memory(file_shmem, file_size, numa_all_nodes_ptr); */
  /*   } */
  rewind(fp);
  
  BARRIER;

  ONCE
    {
      PRINT_ONCE("Opened file %s\n", fn);
      size_t i = 0;
      for (i = 0; i < 27; i++)
	{
	  stats[i] = 0;
	}
      *chunk_index = 0;

#if defined(PRELOAD_MEMORY)
      size_t ten_perc = file_size / 10;

      if (local_data == 0)
	{
	  PRINT_ONCE("Loading file:");
	  i = 0;
	  char c;
	  while ((c = fgetc(fp)) != EOF)
	    {
	      file_shmem[i++] = c;
	      if (i % (ten_perc) == 0)
		{
		  printf("%4lu%% - ", (unsigned long) 10 * i / (ten_perc));
		  FLUSH;
		}
	    }
	  ONCE { printf("\n"); }
	}
      else
	{
	  PRINT_ONCE("Loading with local data");
	  memset((void*) file_shmem, 'x', file_size);
	}
#endif
    }
  file_shmem[file_size] = EOF;


  ONCE
    {
      printf("MapReduce --\n");
      printf("Fillename \t: %s\n", filename);
      printf("Filesize  \t: %lu bytes / %lu MB\n", (unsigned long) file_size, (unsigned long) file_size / (1024 * 1024));
      printf("Chunksize \t: %d bytes\n", chunk_size);
      printf("Chunk num \t: %lu\n", (unsigned long) chunk_num);
      FLUSH;
    }

  BARRIER;

  if (sequential)
    {
      map_reduce_seq(fp, chunk_index, stats, file_shmem, file_size);
    }
  else
    {
      map_reduce(fp, chunk_index, stats, chunks_per_core, file_shmem, file_size);
    }

  fclose(fp);
  BARRIER;

  ONCE
    {
      printf("TOTAL stats - - - - - - - - - - - - - - - -\n");
      int i;
      for (i = 0; i < 26; i++)
	{
	  if (stats[i])
	    {
	      printf("%c :\t %u\n", 'a' + i, (unsigned) stats[i]);
	    }
	}
      printf("Other :\t %d\n", stats[i]);

      printf("Chuncks per core - - - - - - - - - - - - - - - -\n");
      int chunk_tot = 0;
      for (i = 0; i < TOTAL_NODES(); i++)
	{
	  if (is_app_core(i))
	    {
	      chunk_tot += chunks_per_core[i];
	      printf("%02d : %10d (%4.1f%%)\n", i, chunks_per_core[i], 100.0 * chunks_per_core[i] / chunk_num);
	    }
	}
      printf("Total: %d\n", chunk_tot);
    }


  BARRIER;
  /* #  if defined(FAIRCM) */
  /*   APP_EXEC_ORDER */
  /*     { */
  /*       PRINT("aborts: %8d / duration: %f", tm2c_tx_node->tx_aborted, tm2c_tx_node->tx_duration / (REF_SPEED_GHZ * 1e9)); */
  /*     } */
  /*   APP_EXEC_ORDER_END; */
  /* #  endif */


  PF_KILL(0);
  TM_END;
  exit(0);
}


size_t cc;

static inline size_t
get_next_chunk(int* chunk_index)
{
  size_t ci;

  /* cc +=  (NUM_UES - 1); */
  /* ci = cc; */

  TX_START;
  TX_LOAD_STORE(chunk_index, +, 1, TYPE_INT);
  ci = *chunk_index;
  TX_COMMIT_NO_PUB;
  return ci;
}


/*
 */
void
map_reduce(FILE *fp, int *chunk_index, int *stats, int* chunks_per_core, char* file_shmem, size_t file_size)
{
  cc = NODE_ID() - 1;
  size_t ci, chunks_num = 0;

  duration__ = wtime();

  ci = get_next_chunk(chunk_index);

#if defined(PRELOAD_MEMORY)
  size_t chunk = ci * chunk_size;

  char c;
  while (chunk < file_size)
    {
      PRINTD("Handling chuck %d", ci);
      chunks_num++;

      PF_START(1);
      /* size_t index = 1; */

      /* c = file_shmem[chunk++]; */
      /* while (index++ < chunk_size && c != EOF) */
      /* 	{ */
      /* 	  stats_local[char_offset(c)]++; */
      /* 	  c = file_shmem[chunk++]; */
      /* 	} */
      size_t index = 0;
      while (index++ < chunk_size && chunk++ < file_size)
	{
	  c = file_shmem[chunk];
	  stats_local[char_offset(c)]++;
	}
      PF_STOP(1);

      asm volatile ("nop");

      PF_START(0);
      ci = get_next_chunk(chunk_index);
      PF_STOP(0);

      chunk = ci * chunk_size;
    }
#else
  char c;
  while (!fseek(fp, ci * chunk_size, SEEK_SET) && c != EOF)
    {
      PRINTD("Handling chuck %d", ci);
      chunks_num++;

      PF_START(0);
      int index = 0;
      while (index++ < chunk_size && (c = fgetc(fp)) != EOF)
	{
	  stats_local[char_offset(c)]++;
	}
      PF_STOP(0);


      TX_START;
      TX_LOAD_STORE(chunk_index, +, 1, TYPE_INT);
      ci = *chunk_index;
      TX_COMMIT_NO_PUB;
    }
  rewind(fp);
#endif
  duration__ = wtime() - duration__;

  PRINTD("Updating the statistics");
  int i;

  TX_START;
  for (i = 0; i < 27; i++)
    {
      if (stats_local[i])
	{
	  TX_LOAD_STORE(&stats[i], +, stats_local[i], TYPE_INT);
	}
    }
  TX_COMMIT_NO_PUB;

  chunks_per_core[NODE_ID()] = chunks_num;


  for (i = 0; i < 27; i++)
    {
      PRINTF("%c : %d\n", 'a' + i, stats_local[i]);
    }
  FLUSH;
}


/*
 */
void
map_reduce_seq(FILE *fp, int *chunk_index, int *stats, char* file_shmem, size_t file_size)
{
  size_t ci, chunks_num = 0;
  rewind(fp);

  duration__ = wtime();

  size_t i = 0;
#if defined(PRELOAD_MEMORY)
  ci = (*chunk_index)++;
  size_t chunk = ci * chunk_size;

  char c;
  while (chunk < file_size)
    {
      PRINTD("Handling chuck %d", ci);
      chunks_num++;

      PF_START(1);
      size_t index = 0;
      while (index++ < chunk_size && chunk++ < file_size)
	{
	  c = file_shmem[chunk];
	  stats_local[char_offset(c)]++;
	}
      PF_STOP(1);
      ci = (*chunk_index)++;
      chunk = ci * chunk_size;
    }
  /* char c = file_shmem[i++]; */
  /* while (i < file_size) */
  /* 	{ */
  /* 	  stats_local[char_offset(c)]++; */
  /* 	  c = file_shmem[i++]; */
  /* 	} */
#else
  ci = (*chunk_index)++;

  char c;
  while (!fseek(fp, ci * chunk_size, SEEK_SET) && c != EOF)
    {
      PRINTD("Handling chuck %d", ci);

      int index = 0;
      while (index++ < chunk_size && (c = fgetc(fp)) != EOF)
	{
	  stats_local[char_offset(c)]++;
	}

      ci = (*chunk_index)++;
    }

  /* char c; */
  /* while ((c = fgetc(fp)) != EOF) */
  /*   { */
  /*     stats_local[char_offset(c)]++; */
  /*   } */
  rewind(fp);
#endif
  duration__ = wtime() - duration__;

  PRINTD("Updating the statistics");

  for (i = 0; i < 27; i++)
    {
      stats[i] += stats_local[i];
      PRINTF("%c : %d\n", 'a' + i, stats_local[i]);
    }
  FLUSH;
}

