CC?=cc
PREFIX?=/usr/local
CPPFLAGS=-I$(PREFIX)/include -I./libpidutil
LDFLAGS=-L$(PREFIX)/lib -L./libpidutil
LDLIBS=-lpidutil
CFLAGS=-Wall -fno-strict-aliasing
CFLAGS+=$(CPPFLAGS)

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
CFLAGS += -DNORT
else
LDFLAGS += -lrt
endif

.PHONY: libpidutil get-deps

all: get-deps libpidutil ifstatd_

ifstatd_:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ifstatd.c -o ifstatd.o
	$(CC) $(CPPFLAGS) $(LDFLAGS) -g -o ifstatd_ ifstatd.o libpidutil/libpidutil.a 

get-deps:
	git submodule update --init

libpidutil:
	$(MAKE) -C libpidutil all

clean:
	rm -f *.BAK *.log *.o *.a core ifstatd_
	$(MAKE) -C libpidutil clean
