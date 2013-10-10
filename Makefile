# Main Makefile for TM2C

# For platform, choose one out of: DEFAULT, OPTERON, XEON, NIAGARA, TILERA
UNAME := $(shell uname -n)

PLATFORM=DEFAULT

ifeq ($(UNAME), lpd48core)
PLATFORM=OPTERON
TM2C_MAX_PROCS=48
endif

ifeq ($(UNAME), maglite)
PLATFORM=NIAGARA
TM2C_MAX_PROCS=64
endif

ifeq ($(UNAME), parsasrv1.epfl.ch)
PLATFORM=TILERA
TM2C_MAX_PROCS=36
endif

ifeq ($(UNAME), marc041)
PLATFORM=SCC
TM2C_MAX_PROCS=48
endif

ifeq ($(UNAME), diassrv8)
PLATFORM=XEON
TM2C_MAX_PROCS=80
endif

ifeq ($(PLATFORM), DEFAULT)
CORE_SPEED_KHz := $(shell cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq)
ifneq ($(CORE_SPEED_KHz), )
CORE_SPEED := $(shell echo "scale=3; ${CORE_SPEED_KHz}/1000000" | bc -l)
$(info ********************************** I will use the default frequency of: $(CORE_SPEED) GHz)
$(info ********************************** Is this correct? If not, fix it in common.h)
EXTRA_DEFINES += -DREF_SPEED_GHZ=${CORE_SPEED}
endif
endif

.PHONY: all 

all:

TOP := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

SRCPATH := $(TOP)/src
MAININCLUDE := $(TOP)/include

INCLUDES := -I$(MAININCLUDE) -I$(TOP)/external/include
LIBS := -L$(TOP)/external/lib \
		-lm \
		$(PLATFORM_LIBS)

# EXTRA_DEFINES are passed through the command line
DEFINES := $(PLATFORM_DEFINES) $(EXTRA_DEFINES)

## Archive ##
ARCHIVE_SRCS_PURE:= tm2c_app.c tm2c.c tm2c_log.c tm2c_dsl.c tm2c_mem.c \
			measurements.c tm2c_dsl_ht.c tm2c_cm.c tm2c_tx_meta.c

-include settings

ifeq ($(VERSION),DEBUG)
$(info *********************************** debug version)
DEBUG_FLAGS := -O0 -g -ggdb -fno-inline #-DDEBUG 
endif

ifeq ($(TM2C_MAX_PROCS),)
TM2C_MAX_PROCS=${MAX_PROCS}
endif

ifeq ($(HASHTABLE),USE_HASHTABLE_SSHT)
ARCHIVE_SRCS_PURE += ssht.c
endif

# if BACKOFF_RETRY is set, the BACKOFF-RETRY contention management scheme is used. 
# This is similar to the TCP-IP exponentional increasing backoff and retry. 
# When BACKOFF_MAX = infinity -> then every tx is expected to terminate whp.
# Upon a conflict, the transaction aborts and wait for the backoff time before
# retrying.

PLATFORM_DEFINES +=	-DBACKOFF_MAX=${BACKOFF_MAX} \
		       	-DBACKOFF_DELAY=${BACKOFF_DELAY} \
			-D${CONTENTION_MANAGER} \
			-DDSL_CORES_ASSIGN=${DSL_CORES_ASSIGN} \
			-DDSL_PER_NODE=${DSL_PER_NODE} \
			-DADDR_TO_DSL_SEL=${ADDR_TO_DSL_SEL} \
			-DADDR_SHIFT_MASK=${ADDR_SHIFT_MASK} \
			-DTM2C_MAX_PROCS=${TM2C_MAX_PROCS} \
			-DTM2C_SHMEM_SIZE_MB=${TM2C_SHMEM_SIZE_MB}

ifeq ($(BENCHMARK_SYSTEM),1)
$(info ** Benchmarking TM2C)
PLATFORM_DEFINES += -DDO_TIMINGS
endif

ifeq ($(SSHT_BIT_OPTS),1)
$(info ** Using SSHT with BIT operations. Warning: supports up to 64 cores)
PLATFORM_DEFINES += -DBIT_OPTS
endif

ifeq ($(GREEDY_GLOBAL_TS),1)
$(info ** Offset-greedy with global timestamp)
PLATFORM_DEFINES += -DGREEDY_GLOBAL_TS
endif

ifeq ($(PGAS),1)
$(info ** PGAS memory instead of shared memory)
PLATFORM_DEFINES += -DPGAS -DEAGER_WRITE_ACQ
endif

ifeq ($(EAGER_WRITE_ACQ),1)
$(info ** Eager write-lock acquire)
PLATFORM_DEFINES += -DEAGER_WRITE_ACQ
endif

PLATFORM_DEFINES += -DSSHT_DBG_UTILIZATION_DTL=${SSHT_DBG_UTILIZATION_DETAIL}
ifeq ($(SSHT_DBG_UTILIZATION),1)
$(info ** Debug the utilization of the DSL hash table)
PLATFORM_DEFINES += -DSSHT_DBG_UTILIZATION
endif

ifeq ($(NO_SYNC_RESP),1)
$(info ** Use no synchronization for messages when it can be avoided)
PLATFORM_DEFINES += -DSSMP_NO_SYNC_RESP
endif

# Include the platform specific Makefile
# This file has the modifications related to the current platform
-include makefiles/Makefile.$(PLATFORM)

## Apps ##
APPS_DIR := apps
# add the non PGAS applications only if PGAS is not defined
ifneq ($(PGAS),1)
APPS = tm1 tm2 tm3 tm4 tm5 tm6 tm7 tm8 tm9
else
ARCHIVE_SRCS_PURE += pgas_dsl.c pgas_app.c
endif

APPS_SHM_PLUS_PGAS = tm1 tm2 tm3 tm4 tm5 tm6 tm7 tm8 tm9

## Benchmarks ##
BMARKS_DIR := bmarks

MB_LL := microbench/linkedlist
MB_LLPGAS := microbench/linkedlistpgas
MB_HT := microbench/hashtable
MB_HTPGAS := microbench/hashtablepgas
MR := mapreduce

LLFILES = linkedlist test
HTFILES = hashtable intset test

# add the non PGAS applications only if PGAS is not defined
ifneq ($(PGAS),1)
# all bmarks that need to be built
ALL_BMARKS = bank mbll mbht mr mp

# benchmarks if PGAS
else
ALL_BMARKS = bankpgas mbllpgas mbhtpgas mp
endif 

BMARKS_SHM_PLUS_PGAS = bank mbll mbht mr mp bankpgas mbllpgas mbhtpgas mp

## The rest of the Makefile ##

# define the compiler now, if platform makefile exported something through
# CCOMPILE
ifneq (,$(CCOMPILE))
C := $(CCOMPILE)
else
C := gcc
endif

CDEP := $(C)
AR := ar
LD := ld

TM2C_ARCHIVE:= $(TOP)/libtm2c.a

### Pretty output control ###
# Set up compiler and linker commands that either is quiet (does not print 
# the command line being executed) or verbose (print the command line). 
_C := $(C)
_CDEP := $(CDEP)
_LD := $(LD)
_AR := $(AR)

ifeq ($(VERBOSE),1)
 override C = $(_C)
 override LD = $(_LD)
 override CDEP = $(_CDEP)
 override AR = $(_AR)
else
 override C    = @echo "    COMPILE         $<"; $(_C)
 override LD   = @echo "    LINKING         $@"; $(_LD)
 override CDEP = @echo "    DEPGEN          $@"; $(_CDEP)
 override AR   = @echo "    ARCHIVE         $@ ($^)"; $(_AR)
endif

CFLAGS := -Wall

$(info ** Use hastable $(HASHTABLE))
EXTRA_CFLAGS += -D$(HASHTABLE)

LDFLAGS := $(LDFLAGS) $(PLATFORM_LDFLAGS)
LIBS    := $(LIBS) $(PLATFORM_LIBS)
CFLAGS  := $(CFLAGS) $(DEFINES) $(PLATFORM_DEFINES) \
		   $(PLATFORM_CFLAGS) \
		   $(DEBUG_FLAGS) $(INCLUDES) $(EXTRA_CFLAGS)

.PHONY: all libs clean install dist

all: archive applications benchmarks

## Archive specific stuff ##
ARCHIVE_SRCS := $(addprefix $(SRCPATH)/, $(ARCHIVE_SRCS_PURE))
ARCHIVE_OBJS = $(ARCHIVE_SRCS:.c=.o)
ARCHIVE_DEPS = $(ARCHIVE_SRCS:.c=.d)

$(SRCPATH)/%.o:: $(SRCPATH)/%.c settings
	$(C) $(CFLAGS) -o $@ -c $<

# We do this only on the SCC with ssmp
ifeq ($(PLATFORM),SCC)
$(ARCHIVE):
	@(cd $(RCCEPATH) ; make API=gory)
endif


$(TM2C_ARCHIVE): $(ARCHIVE) $(ARCHIVE_OBJS) makefiles/Makefile.$(PLATFORM) settings
	@echo Archive name = $(TM2C_ARCHIVE) 
ifeq ($(PLATFORM),SCC)
	@rm -rf .ar-lib
	@mkdir .ar-lib
	@(cd .ar-lib && ar x $(ARCHIVE))
	@$(AR) cr $(TM2C_ARCHIVE) $(ARCHIVE_OBJS) .ar-lib/*.o
	@rm -rf .ar-lib
else
	$(AR) cr $(TM2C_ARCHIVE) $(ARCHIVE_OBJS)
endif
	ranlib $(TM2C_ARCHIVE)

archive: $(TM2C_ARCHIVE)

## Apps specific stuff ##

APPS := $(addprefix $(APPS_DIR)/,$(APPS))
APP_OBJS = $(addsuffix .o,$(APPS))
APP_SRCS = $(addsuffix .c,$(APPS))
APP_DEPS = $(addsuffix .d,$(APPS))

$(APPS): $(TM2C_ARCHIVE) $(APP_SRCS)

# an exception
$(APPS_DIR)/%: $(APPS_DIR)/%.c
	$(C) $(CFLAGS) $(LDFLAGS) -o $@ $< $(TM2C_ARCHIVE) $(LIBS)

applications: $(TM2C_ARCHIVE) $(APPS)

## Benchmarks specific stuff ##
ALL_BMARK_FILES = $(BMARKS) \
				  $(MR)/mr \
				  $(addprefix $(MB_LL)/,$(LLFILES)) \
				  $(addprefix $(MB_HT)/,$(HTFILES))

# if there is PGAS
ifeq ($(PGAS),1)
ALL_BMARK_FILES += \
					$(addprefix $(MB_LLPGAS)/,$(LLFILES)) \
					$(addprefix $(MB_HTPGAS)/,$(HTFILES))
endif

ALL_BMARK_FILES := $(addprefix $(BMARKS_DIR)/, $(ALL_BMARK_FILES))
BMARKS_SHM_PLUS_PGAS := $(addprefix $(BMARKS_DIR)/, $(BMARKS_SHM_PLUS_PGAS))
BMARKS_OBJS = $(addsuffix .o,$(ALL_BMARK_FILES)) 
BMARKS_SRCS = $(addsuffix .c,$(ALL_BMARK_FILES))
BMARKS_DEPS = $(addsuffix .d,$(ALL_BMARK_FILES))

APPS_SHM_PLUS_PGAS := $(addprefix $(APPS_DIR)/, $(APPS_SHM_PLUS_PGAS))

# set the proper directory
ALL_BMARKS := $(addprefix $(BMARKS_DIR)/, $(ALL_BMARKS))
BMARKS_DEPS += $(addsuffix .d,$(ALL_BMARKS))

$(ALL_BMARKS): $(TM2C_ARCHIVE)

$(BMARKS_DIR)/%.o:: $(BMARKS_DIR)/%.c
	$(C) $(CFLAGS) -o $@ -c $<

$(BMARKS_DIR)/%: $(BMARKS_DIR)/%.c
	$(C) $(CFLAGS) $(LDFLAGS) -o $@ $< $(TM2C_ARCHIVE) $(LIBS)

$(BMARKS_DIR)/mbll: $(filter $(BMARKS_DIR)/$(MB_LL)/%,$(BMARKS_OBJS)) $(TM2C_ARCHIVE)
	$(C) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

$(BMARKS_DIR)/mbht: $(filter $(BMARKS_DIR)/$(MB_HT)/%,$(BMARKS_OBJS)) $(BMARKS_DIR)/$(MB_LL)/linkedlist.o $(TM2C_ARCHIVE)
	$(C) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

$(BMARKS_DIR)/mbllpgas: $(filter $(BMARKS_DIR)/$(MB_LLPGAS)/%,$(BMARKS_OBJS)) $(TM2C_ARCHIVE)
	$(C) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

$(BMARKS_DIR)/mbhtpgas: $(filter $(BMARKS_DIR)/$(MB_HTPGAS)/%,$(BMARKS_OBJS)) $(BMARKS_DIR)/$(MB_LLPGAS)/linkedlist.o $(TM2C_ARCHIVE)
	$(C) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

$(BMARKS_DIR)/mr: $(filter $(BMARKS_DIR)/$(MR)/%,$(BMARKS_OBJS)) $(TM2C_ARCHIVE)
	$(C) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

benchmarks: $(ALL_BMARKS)

clean_archive:
	@echo "Cleaning archives (libraries)..."
	rm -f $(ARCHIVE_OBJS)
	rm -f $(TM2C_ARCHIVE)

clean_apps:
	@echo "Cleaning apps dir..."
	rm -f $(APP_OBJS) $(APPS_SHM_PLUS_PGAS)

clean_bmarks:
	@echo "Cleaning benchmarks dir..."
	rm -f $(BMARKS_OBJS) $(BMARKS_SHM_PLUS_PGAS)

clean: clean_archive clean_apps clean_bmarks

realclean: clean
	rm -f $(ARCHIVE_DEPS)
	rm -f $(APP_DEPS)
	rm -f $(BMARKS_DEPS)

.PHONY: clean clean_bmarks clean_apps clean_archive realclean

depend: $(ARCHIVE_DEPS) $(APP_DEPS) $(BMARKS_DEPS)

ifeq (,$(filter %clean,$(MAKECMDGOALS)))
-include $(ARCHIVE_DEPS)
-include $(APP_DEPS)
-include $(BMARKS_DEPS)
endif

