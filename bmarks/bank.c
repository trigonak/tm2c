/*
 * Adapted to TM2C by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch> 
 *
 * File:
 *   bank.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Patrick Marlier <patrick.marlier@unine.ch>
 * Description:
 *   Bank stress test.
 *
 * Copyright (c) 2007-2011.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <malloc.h>

#include "tm2c.h"

/*
 * Useful macros to work with transactions. Note that, to use nested
 * transactions, one should check the environment returned by
 * stm_get_env() and only call sigsetjmp() if it is not null.
 */

#define NB_ACC_POWER2
#define LOAD_STORE

#define DEFAULT_DURATION                2
#define DEFAULT_DELAY                   0
#define DEFAULT_NB_ACCOUNTS             1024
#define DEFAULT_NB_THREADS              1
#define DEFAULT_READ_ALL                0
#define DEFAULT_CHECK                   0
#define DEFAULT_WRITE_ALL               0
#define DEFAULT_READ_THREADS            0
#define DEFAULT_WRITE_THREADS           0
#define DEFAULT_DISJOINT                0
#define DEFAULT_VERBOSE                 0

int delay = DEFAULT_DELAY;
int test_verbose = DEFAULT_VERBOSE;

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define CAST_VOIDP(addr)                ((void *) (addr))

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

/* ################################################################### *
 * BANK ACCOUNTS
 * ################################################################### */

typedef struct account 
{
  int32_t number;
  int32_t balance;
  /* uint8_t padding[64 - 8]; */
} account_t;

typedef struct bank 
{
  account_t* accounts;
  uint32_t size;
} bank_t;

int 
transfer(account_t* src, account_t* dst, int amount) 
{
  /* PRINT("in transfer"); */

  /* Allow overdrafts */
  TX_START;

#ifdef LOAD_STORE
  TX_LOAD_STORE(&src->balance, -, amount, TYPE_INT);
  TX_LOAD_STORE(&dst->balance, +, amount, TYPE_INT);

  TX_COMMIT_NO_PUB;
  /* PF_STOP(3); */
#else
  int i, j;
  i = *(int *) TX_LOAD(&src->balance);
  i -= amount;
  TX_STORE(&src->balance, &i, TYPE_INT); //NEED TX_STOREI
  j = *(int *) TX_LOAD(&dst->balance);
  j += amount;
  TX_STORE(&dst->balance, &j, TYPE_INT);
  TX_COMMIT;
#endif

  return amount;
}

void
check_accs(account_t* acc1, account_t* acc2) 
{
  /* PRINT("in check_accs"); */

  volatile int i, j;
  
  TX_START;
  i = *(int *) TX_LOAD(&acc1->balance);
  j = *(int *) TX_LOAD(&acc2->balance);

  TX_COMMIT;

  if (i == j<<20)
    {
      PRINT("dummy print");
    }
}

int
total(bank_t* bank, int transactional) 
{
  int i, total;

  if (!transactional) 
    {
      total = 0;
      for (i = 0; i < bank->size; i++) 
	{
	  total += bank->accounts[i].balance;
	}
    }
  else
    {
      TX_START;
      total = 0;
      for (i = 0; i < bank->size; i++)
	{
	  int *bal = (int*) TX_LOAD(&bank->accounts[i].balance); 
	  total += *bal;
	}
      TX_COMMIT;
    }

  if (total != 0) 
    {
      PRINT("Got a bank total of: %d", total);
    }

  return total;
}

void
reset(bank_t *bank) 
{
  int i;

  TX_START;
  for (i = 0; i < bank->size; i++) 
    {
      TX_STORE(&bank->accounts[i].balance, 0, TYPE_INT);
    }
  TX_COMMIT;
}


/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data 
{
  uint64_t nb_transfer;
  uint64_t nb_checks;
  uint64_t nb_read_all;
  uint64_t nb_write_all;
  int32_t id;
  int32_t read_cores;
  int32_t write_cores;
  int32_t read_all;
  int32_t write_all;
  int32_t check;
  int32_t nb_app_cores;
  int32_t padding;
} thread_data_t;


volatile int work = 1;

void
alarm_handler(int sig)
{
  work = 0;
}

bank_t* 
test(void *data, double duration, int nb_accounts) 
{
  int rand_max;
  uint8_t is_read_core = 0;
  thread_data_t *d = (thread_data_t *) data;
  bank_t * bank;

  if (d->id < d->read_cores)
    {
      is_read_core = 1;
    }

  srand_core();
  BARRIER;


#if defined(NB_ACC_POWER2)
  rand_max = nb_accounts - 1;
#else
  rand_max = nb_accounts;
#endif	/* NB_ACC_POWER2 */

  bank = (bank_t*) malloc(sizeof (bank_t));
  if (bank == NULL) 
    {
      PRINT("malloc bank");
      EXIT(1);
    }

  bank->accounts = (account_t *) sys_shmalloc(nb_accounts * sizeof (account_t));
  if (bank->accounts == NULL) 
    {
      PRINT("malloc bank->accounts");
      EXIT(1);
    }

  bank->size = nb_accounts;

  ONCE
    {
      int i;
      for (i = 0; i < bank->size; i++)
	{
	  NONTX_STORE(&bank->accounts[i].number, i, TYPE_INT);
	  NONTX_STORE(&bank->accounts[i].balance, 0, TYPE_INT);
	}
    }

  ONCE
    {
      uint32_t tot = total(bank, 0);
      if (test_verbose)
	{
	  PRINT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\tBank total (before): %d",
		tot);
	}
    }

  /* Wait on barrier */
  BARRIER;

  /* warming up */
  TX_START;
  TX_COMMIT_NO_STATS;
  total(bank, 0);

  /* you do this only once */
  signal(SIGALRM, alarm_handler);

  PF_MSG(11, "alarm call");

  /* this triggers the alarm after one second */


  alarm(duration);

  BARRIER;

  /* FOR(duration) */
  /* FOR_ITERS(1000000) */
 
  ticks __start_ticks = getticks();
  while(work)
    {
      uint8_t nb = tm2c_rand() & 127;
      if (is_read_core || nb < d->read_all)
	{
	  /* Read all */
	  total(bank, 1);
	  d->nb_read_all++;
	}
      else
	{
	  /* Choose random accounts */

#if defined(NB_ACC_POWER2)
	  uint32_t src = tm2c_rand() & rand_max;
	  uint32_t dst = tm2c_rand() & rand_max;
#else
	  uint32_t src = tm2c_rand() % rand_max;
	  uint32_t dst = tm2c_rand() % rand_max;
#endif	/* NB_ACC_POWER2 */
	  if (dst == src)
	    {
#if defined(NB_ACC_POWER2)
	      dst = ((src + 1) & rand_max);
#else
	      dst = ((src + 1) % rand_max);
#endif	/* NB_ACC_POWER2 */
	    }
	  if (nb < d->check) 
	    {
	      check_accs(bank->accounts + src, bank->accounts + dst);
	      d->nb_checks++;
	    }
	  else 
	    {
	      transfer(bank->accounts + src, bank->accounts + dst, 1);
	      d->nb_transfer++;
	    }
	}
    }
  ticks __end_ticks = getticks();
  ticks __duration_ticks = __end_ticks - __start_ticks;
  ticks __ticks_per_sec = (ticks) (1e9 * REF_SPEED_GHZ);
  duration__ = (double) __duration_ticks / __ticks_per_sec;

  BARRIER;

  return bank;
}


int
main(int argc, char **argv)
{
  dup2(STDOUT_FILENO, STDERR_FILENO);
  PF_MSG(0, "1st TX_LOAD_STORE (transfer)");
  PF_MSG(1, "2nd TX_LOAD_STORE (transfer)");
  PF_MSG(2, "the whole transfer");

  TM2C_INIT;

  struct option long_options[] =
    {
      // These options don't set a flag
      {"help", no_argument, NULL, 'h'},
      {"accounts", required_argument, NULL, 'a'},
      {"contention-manager", required_argument, NULL, 'c'},
      {"duration", required_argument, NULL, 'd'},
      {"delay", required_argument, NULL, 'D'},
      {"read-all-rate", required_argument, NULL, 'r'},
      {"check", required_argument, NULL, 'c'},
      {"read-threads", required_argument, NULL, 'R'},
      {"write-all-rate", required_argument, NULL, 'w'},
      {"write-threads", required_argument, NULL, 'W'},
      {"verbose", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0}
    };

  bank_t* bank;
  int i, c;
  thread_data_t* data;

  double duration = DEFAULT_DURATION;
  int nb_accounts = DEFAULT_NB_ACCOUNTS;
  int nb_app_cores = NUM_APP_NODES;
  int read_all = DEFAULT_READ_ALL;
  int read_cores = DEFAULT_READ_THREADS;
  int write_all = DEFAULT_READ_ALL + DEFAULT_WRITE_ALL;
  int check = write_all + DEFAULT_CHECK;
  int write_cores = DEFAULT_WRITE_THREADS;

  while (1)
    {
      i = 0;
      c = getopt_long(argc, argv, "ha:d:D:r:c:R:w:W:jv", long_options, &i);

      if (c == -1)
	break;

      if (c == 0 && long_options[i].flag == 0)
	c = long_options[i].val;

      switch (c)
	{
	case 0:
	  /* Flag is automatically set */
	  break;
	case 'h':
	  ONCE
	    {
	      PRINT("bank -- STM stress test\n"
		    "\n"
		    "Usage:\n"
		    "  bank [options...]\n"
		    "\n"
		    "Options:\n"
		    "  -h, --help\n"
		    "        Print this message\n"
		    "  -a, --accounts <int>\n"
		    "        Number of accounts in the bank (default=" XSTR(DEFAULT_NB_ACCOUNTS) ")\n"
		    "  -d, --duration <double>\n"
		    "        Test duration in seconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
		    "  -d, --delay <int>\n"
		    "        Delay ns after succesfull request. Used to understress the system, default=" XSTR(DEFAULT_DELAY) ")\n"
		    "  -c, --check <int>\n"
		    "        Percentage of check transactions transactions (default=" XSTR(DEFAULT_CHECK) ")\n"
		    "  -r, --read-all-rate <int>\n"
		    "        Percentage of read-all transactions (default=" XSTR(DEFAULT_READ_ALL) ")\n"
		    "  -R, --read-threads <int>\n"
		    "        Number of threads issuing only read-all transactions (default=" XSTR(DEFAULT_READ_THREADS) ")\n"
		    "  -w, --write-all-rate <int>\n"
		    "        Percentage of write-all transactions (default=" XSTR(DEFAULT_WRITE_ALL) ")\n"
		    "  -W, --write-threads <int>\n"
		    "        Number of threads issuing only write-all transactions (default=" XSTR(DEFAULT_WRITE_THREADS) ")\n"
		    );
	    }
	  exit(0);
	case 'a':
	  nb_accounts = atoi(optarg);
	  break;
	case 'd':
	  duration = atof(optarg);
	  break;
	case 'D':
	  delay = atoi(optarg);
	  break;
	case 'c':
	  check = atoi(optarg);
	  break;
	case 'r':
	  read_all = atoi(optarg);
	  break;
	case 'R':
	  read_cores = atoi(optarg);
	  break;
	case 'w':
	  write_all = atoi(optarg);
	  PRINT("*** warning: write all has been disabled");
	  break;
	case 'W':
	  write_cores = atoi(optarg);
	  PRINT("*** warning: write all cores have been disabled");
	  break;
	case 'v':
	  test_verbose = 1;
	  break;
	case '?':
	  ONCE
	    {
	      PRINT("Use -h or --help for help\n");
	    }

	  exit(0);
	default:
	  exit(1);
	}
    }


#if defined(NB_ACC_POWER2)
  nb_accounts = pow2roundup(nb_accounts);
#endif	/* NB_ACC_POWER2 */
  write_all = 0;
  write_cores = 0;

  write_all += read_all;
  check += write_all;

  assert(duration >= 0);
  assert(nb_accounts >= 2);
  assert(nb_app_cores > 0);
  assert(read_all >= 0 && write_all >= 0 && check >= 0 && check <= 100);
  assert(read_cores + write_cores <= nb_app_cores);

  if (test_verbose)
    {
      ONCE
	{
	  PRINTN("Nb accounts    : %d\n", nb_accounts);
	  PRINTN("Duration       : %fs\n", duration);
	  PRINTN("Check acc rate : %d\n", check - write_all);
	  PRINTN("Transfer rate  : %d\n", 100 - check);
	}
    }
  /* normalize percentages to 128 */

  double normalize = (double) 128/100;
  check *= normalize;
  write_all *= normalize;
  read_all *= normalize;


  data = (thread_data_t*) memalign(CACHE_LINE_SIZE, sizeof(thread_data_t));
  if (data == NULL)
    {
      perror("posix_memalign");
      exit(1);
    }

  /* Init STM */
  BARRIER;

  data->id = app_id_seq(NODE_ID());
  data->check = check;
  data->read_all = read_all;
  data->write_all = write_all;
  data->read_cores = read_cores;
  data->write_cores = write_cores;
  data->nb_app_cores = nb_app_cores;
  data->nb_transfer = 0;
  data->nb_checks = 0;
  data->nb_read_all = 0;
  data->nb_write_all = 0;

  BARRIER;

  bank = test(data, duration, nb_accounts);

  /* End it */

  BARRIER;

  if (test_verbose)
    {
      APP_EXEC_ORDER
	{
	  printf("---Core %d\n  #transfer   : %llu\n  #checks     : %llu\n  #read-all   : %llu\n  #write-all  : %llu\n", 
		 NODE_ID(), (LLU) data->nb_transfer, (LLU) data->nb_checks, (LLU) data->nb_read_all, (LLU) data->nb_write_all);
	  FLUSH;
	} APP_EXEC_ORDER_END;
    }

  BARRIER;

  ONCE
    {
      uint32_t tot = total(bank, 0);
      PRINT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\tBank total  (after): %u", tot);
    }

  /* Delete bank and accounts */
  sys_shfree((void**) bank->accounts);
  free(bank);
  free(data);

  TM_END;

  EXIT(0);
}
