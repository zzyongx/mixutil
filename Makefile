## Author: iamzhengzhiyong@gmail.com

CC      = gcc
CXX     = g++
AR      = ar
MKDIR   = mkdir
INSTALL = install

PREDEF   = -D_FILE_OFFSET_BITS=64
CFLAGS   = -I./include -std=c99
CXXFLAGS = -I./include
CXXWARN  = -Wno-invalid-offsetof
LDFLAGS  = -lpth -ldl

ifeq ($(INDEX_HASH_DEBUG), 1)
	PREDEF += -DINDEX_HASH_DEBUG
endif

ifeq ($(DEBUG), 1)
	CFLAGS   += -O0 -g
  CXXFLAGS += -O0 -g
  WARN     += -Wall -Wextra -Wno-comment -Wformat -Wimplicit \
			        -Wparentheses -Wswitch -Wunused
else
  CFLAGS   += -O2 -g
  CXXFLAGS += -O2 -g
  WARN     += -Wall -Wextra -Wno-comment -Wformat -Wimplicit \
	     		    -Wparentheses -Wswitch -Wuninitialized -Wunused
endif

ifndef ($(INSTALLDIR))
	INSTALLDIR = /usr/local
endif

all: configure build

.PHONY: configure
configure:
	$(MKDIR) -p build

OBJS    = build/util.o build/crc.o build/index_hash.o
NBNET_OBJS = build/nbnet.o build/index_hash.o

.PHONY: build
build: $(OBJS)
	$(AR) -rcs ./build/libmixutil.a $(OBJS)

build-nbnet: $(NBNET_OBJS)
	$(CXX) -shared -o ./build/libnbnet.so $(NBNET_OBJS) $(LDFLAGS)

build/%.o: src/%.cc
	$(CXX) -o $@ $(WARN) $(CXXWARN) $(CXXFLAGS) $(PREDEF) -c $<

build/%.o: src/%.c
	$(CC) -o $@ $(WARN) $(CFLAGS) $(PREDEF) -c $<

.PHONY: test
TEST_OBJS = build/util_test.o build/logger_test.o build/index_hash_test.o
TEST_LDFLAGS = -lgtest -lgtest_main -lpthread
TEST_NBNET_LDFLAGS = -lmysqlclient -L/usr/lib/mysql -lnbnet -L./build
TEST_CXXFLAGS = -std=c++0x

build-test: configtest $(TEST_OBJS) $(OBJS) build/nbnet_test.o build-nbnet
	$(CXX) -o ./build/runtest $(TEST_OBJS) $(OBJS) $(TEST_LDFLAGS)
	$(CXX) -o ./build/nbnet_test build/nbnet_test.o $(OBJS) $(LDFLAGS) $(TEST_LDFLAGS) $(TEST_NBNET_LDFLAGS)

test: build-test
	./build/runtest
	LD_PRELOAD=$PWD/build/libnbnet.so LD_LIBRARY_PATH=$PWD/build ./build/nbnet_test

build/%.o: test/%.cc
	$(CXX) -o $@ $(WARN) $(CXXWARN) $(CXXFLAGS) $(PREDEF) $(TEST_CXXFLAGS) -c $<

configtest:
	$(eval PREDEF += -DUNIT_TEST)

.PHONY: install
install:
	$(INSTALL) -D build/libmixutil.a   $(DESTDIR)$(INSTALLDIR)/lib/libmixutil.a
	$(MKDIR)   -p $(DESTDIR)$(INSTALLDIR)/include/mixutil
	$(INSTALL) -m 0644 include/mixutil/*.h $(DESTDIR)$(INSTALLDIR)/include/mixutil/

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
