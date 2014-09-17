## Author: iamzhengzhiyong@gmail.com

CC      = gcc
CXX     = g++
AR      = ar
MKDIR   = mkdir
INSTALL = install

CFLAGS  = -I./include
PREDEF  = -D_FILE_OFFSET_BITS=64
CXXWARN = -Wno-invalid-offsetof
LDFLAGS = 

ifeq ($(DEBUG), 1)
    CFLAGS += -O0 -g
    WARN   += -Wall -Wextra -Wno-comment -Wformat -Wimplicit \
			  -Wparentheses -Wswitch -Wunused
else
    CFLAGS += -O2 -g
    WARN   += -Wall -Wextra -Wno-comment -Wformat -Wimplicit \
			  -Wparentheses -Wswitch -Wuninitialized -Wunused
endif

ifndef ($(INSTALLDIR))
	INSTALLDIR = /usr/local
endif

all: configure build

.PHONY: configure
configure:
	$(MKDIR) -p build

OBJS    = build/util.o build/crc.o
.PHONY: build
build: $(OBJS)
	$(AR) -rcs ./build/libmixutil.a $(OBJS) $(LDFLAGS)

build/%.o: src/%.cc
	$(CXX) -o $@ $(WARN) $(CXXWARN) $(CFLAGS) $(PREDEF) -c $<

.PHONY: test
TEST_OBJS = build/util_test.o build/logger_test.o
TEST_LDFLAGS = -lgtest -lgtest_main -lpthread

test: $(TEST_OBJS) $(OBJS)
	$(CXX) -o ./build/runtest $(TEST_OBJS) $(OBJS) $(TEST_LDFLAGS)
	./build/runtest

build/%.o: test/%.cc
	$(CXX) -o $@ $(WARN) $(CXXWARN) $(CFLAGS) $(PREDEF) -c $<

.PHONY: install
install:
	$(INSTALL) -D build/libmixutil.a   $(DESTDIR)$(INSTALLDIR)/lib/libmixutil.a
	$(MKDIR)   -p $(DESTDIR)$(INSTALLDIR)/include/mixutil
	$(INSTALL) -m 0644 include/*.h     $(DESTDIR)$(INSTALLDIR)/include/mixutil/

.PHONY: clean
clean:
	rm -rf ./build/*

.PHONY: help
help:
	@echo "make [cmd] [DEBUG=0|1]"
	@echo "... all  build it"
	@echo "... test make unit test"
	@echo "... clean"
	@echo "... install [INSTALLDIR=_INSTALLDIR_]"
