#
# pretty much always want debugging symbols included
CFLAGS+=-g
# yell out all warnings and whatnot
CFLAGS+=-Wall -Wextra -Wunused
# make all warnings into errors
#CFLAGS+=-Werror
# optimize!
#CFLAGS+=-O3
# or not!
CFLAGS+=-O0
#
# das linker flags
# LDFLAGS+=
#
CC:=gcc
#
.SUFFIXES:
.SUFFIXES: .c .o
#
SHELL=/bin/sh
#
VERSION="$(shell cat VERSION)"
CFLAGS+=-DVERSION='$(VERSION)'
#
CPRG=xboxfs
#
.PHONY: all clean

all: $(CPRG)

% : %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -DPROGNAME='"$@"' -o $@ $<

clean:
	-rm -f $(CPRG)
