# Configurable options
# MODE = release | debug (default: release)
MODE = debug
# Management PC specific settings
CORE_NUM := $(shell nproc)
ifneq ($(CORE_SPEED_KHz), )
CFLAGS += -DCORE_NUM=${CORE_NUM}
else
CFLAGS += -DCORE_NUM=4
endif
CFLAGS += -DDEFAULT
$(info *** Using as a default number of cores: $(CORE_NUM) on 1 socket)
$(info ***)

# Architecture dependent settings
ifndef ARCH
ARCH_NAME = $(shell uname -m)
endif

ifeq ($(ARCH_NAME), i386)
ARCH = x86
CFLAGS += -m32
LDFLAGS += -m32
endif

ifeq ($(ARCH_NAME), i686)
ARCH = x86
CFLAGS += -m32
LDFLAGS += -m32
endif

ifeq ($(ARCH_NAME), x86_64)
ARCH = x86_64
CFLAGS += -m64
LDFLAGS += -m64
endif

# Generic configurations
CFLAGS += --std=gnu99 -pedantic -Wall
CFLAGS += -fno-strict-aliasing
CFLAGS += -D_GNU_SOURCE
CFLAGS += -D_REENTRANT
CFLAGS += -I include
LDFLAGS += -lpthread

ifneq ($(MODE),debug)
	CFLAGS += -O3 -DNDEBUG
else
	CFLAGS += -g
endif

OUT = out
EXEC = $(OUT)/test-lock
all: $(EXEC)

deps =

LOCK_OBJS =
LOCK_OBJS += src/lock/concurrent_main.o

deps += $(LOCK_OBJS:%.o=%.o.d)

$(OUT)/test-lock: $(LOCK_OBJS)
	@mkdir -p $(OUT)
	$(CC) -o $@ $^ $(LDFLAGS)
src/lock/%.o: src/lock/%.c
	$(CC) $(CFLAGS) -DLOCKTYPE -o $@ -MMD -MF $@.d -c $<

#LOCKFREE_OBJS =
#LOCKFREE_OBJS += \
#    src/lockfree/list.o \
#    src/lockfree/main.o
deps += $(LOCKFREE_OBJS:%.o=%.o.d)

#$(OUT)/test-lockfree: $(LOCKFREE_OBJS)
#	@mkdir -p $(OUT)
#	$(CC) -o $@ $^ $(LDFLAGS)
#src/lockfree/%.o: src/lockfree/%.c
#	$(CC) $(CFLAGS) -DLOCKFREE -o $@ -MMD -MF $@.d -c $<
#
check: $(EXEC)
	bash scripts/test_correctness.sh

bench: $(EXEC)
	bash scripts/run_ll.sh
	bash scripts/create_plots_ll.sh >/dev/null
	@echo Check the plots generated in directory 'out/plots'.

clean:
	$(RM) -f $(EXEC)
	$(RM) -f $(LOCK_OBJS) $(LOCKFREE_OBJS) $(deps)

distclean: clean
	$(RM) -rf out

.PHONY: all check clean distclean

-include $(deps)
