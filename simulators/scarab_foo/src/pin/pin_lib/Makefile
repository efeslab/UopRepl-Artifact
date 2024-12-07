OS        := $(shell uname)
BASEDIR    = ..
GNUDIR     = $(BASEDIR)/gnu
CC          = gcc
CCC         = g++
ARCHIVER    = ar
ARCHTYPE    = 
VERSION = 0

SCARAB_DIR ?= ../../
OBJDIR := obj
CCFILES = message_queue_interface_lib.cc pin_scarab_common_lib.cc x86_decoder.cc x87_stack_delta.cc
CFILES = uop_generator.c
OBJFILES = $(patsubst %.cc,$(OBJDIR)/%.o,$(CCFILES)) $(patsubst %.c,$(OBJDIR)/%.o,$(CFILES))
TARGET = libpin_fe

DEBUG_FLAGS = -DNO_DEBUG
XFLAGS	    = -D_REENTRANT -DMEMTRACE
WARN_CFLAGS = -Wall -Wunused -Wmissing-prototypes -Wmissing-declarations -Wno-long-long -Wpointer-arith -Wimplicit -Werror
ifeq ($(CCVER), gcc (GCC) 4.7.3)
WARN_CFLAGS += -Wno-unused-but-set-variable -Wno-maybe-uninitialized
endif
WARN_CCFLAGS= -Wall -Wunused -Wno-long-long -Wpointer-arith -Werror

NO_STAT ?=
PIN_COMPILE ?=
INCLUDES = -I$(SCARAB_DIR) -I$(PIN_ROOT)/
OPTCFLAGS   = -std=gnu99 -O3 -g $(ARCHTYPE) -funroll-loops -Werror $(INCLUDES) $(DEBUG_FLAGS) $(WARN_CFLAGS) $(XFLAGS) $(NO_STAT) $(PIN_COMPILE) 
OPTCCFLAGS   = -std=c++14 -O3 -g $(ARCHTYPE) -fno-inline -funroll-loops -Werror $(INCLUDES) $(DEBUG_FLAGS) $(WARN_CCFLAGS) $(XFLAGS) $(NO_STAT) $(PIN_COMPILE)

DEPDIR := $(OBJDIR)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

.PHONY : clean dir all dbg objects archive

all:
	make dir
	make archive

$(OBJDIR)/$(TARGET).a: $(OBJFILES)
	$(ARCHIVER) rcs $@ $^

$(OBJDIR)/%.o: %.c $(DEPDIR)/%.d
	$(CC) $(DEPFLAGS) $(OPTCFLAGS) -fPIC -c -o $@ $<

$(OBJDIR)/%.o: %.cc $(DEPDIR)/%.d
	$(CCC) $(DEPFLAGS) $(OPTCCFLAGS) -fPIC -c -o $@ $<

dbg: OPTCCFLAGS += -g -DDEBUG
dbg: all

vgr: OPTCFLAGS = -std=gnu99 -O0 -g $(ARCHTYPE) -funroll-loops -Werror $(DEBUG_FLAGS) $(WARN_CFLAGS) $(XFLAGS) $(NO_STAT) $(PIN_COMPILE)# -DNO_ASSERT 
vgr: OPTCCFLAGS = -std=c++14 -O0 -g $(ARCHTYPE) -fno-inline -funroll-loops -Werror $(DEBUG_FLAGS) $(WARN_CCFLAGS) $(XFLAGS) $(NO_STAT) $(PIN_COMPILE)
vgr: |dir archive

objects: $(OBJFILES)

archive: $(OBJDIR)/$(TARGET).a

dir:
	mkdir -p $(OBJDIR)

clean:
	-rm -rf $(OBJDIR)

DEPFILES := $(OBJFILES:$(OBJDIR)/%.o=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
