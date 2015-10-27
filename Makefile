CC?=cc
PREFIX?=/usr/local
CPPFLAGS=-I$(PREFIX)/include
LDFLAGS=-L$(PREFIX)/lib  
LDLIBS=-lpidutil
FLAGS=-Wall -Wextra -O2 -pipe -funroll-loops -ffast-math -fno-strict-aliasing -mssse3

.PHONY: libpidutil get-deps

all: get-deps libpidutil ifstatd_

ifstatd_: ifstatd.o

get-deps:
	git submodule update --init

libpidutil:
	$(MAKE) -C libpidutil all

clean:
	rm -f *.BAK *.log *.o *.a core ifstatd_
	$(MAKE) -C libpidutil clean
