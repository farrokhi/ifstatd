PREFIX=/usr/local
INC=-I$(PREFIX)/include
LIB=-L$(PREFIX)/lib 
FLAGS=-O2 -pipe -funroll-loops -ffast-math -fno-strict-aliasing -mssse3
CC?=cc

all: ifstatd

ifstatd: ifstatd.c Makefile 
	$(CC) $(FLAGS) $(INC) $(LIB) ifstatd.c -o ifstatd

clean:
	rm -f ifstatd

