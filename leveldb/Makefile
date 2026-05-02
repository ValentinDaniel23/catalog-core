UK_ROOT ?= $(CURDIR)/workdir/unikraft
UK_BUILD ?= $(CURDIR)/workdir/build
UK_APP ?= $(CURDIR)
LIBLEVELDB_UPSTREAM ?= $(abspath $(CURDIR)/../../leveldb)
LIBS_BASE = $(CURDIR)/workdir/libs
UK_LIBS := $(LIBS_BASE)/libcxxabi:$(LIBS_BASE)/libcxx:$(LIBS_BASE)/libunwind:$(LIBS_BASE)/compiler-rt:$(LIBS_BASE)/musl:$(LIBS_BASE)/leveldb

.PHONY: all

all:
	@$(MAKE) -C $(UK_ROOT) L=$(UK_LIBS) A=$(UK_APP) O=$(UK_BUILD) LIBLEVELDB_UPSTREAM=$(LIBLEVELDB_UPSTREAM)

$(MAKECMDGOALS):
	@$(MAKE) -C $(UK_ROOT) L=$(UK_LIBS) A=$(UK_APP) O=$(UK_BUILD) LIBLEVELDB_UPSTREAM=$(LIBLEVELDB_UPSTREAM) $(MAKECMDGOALS)
