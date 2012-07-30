#
# pretty much always want debugging symbols included
CXXFLAGS+=-g
# yell out all warnings and whatnot
CXXFLAGS+=-Wall -Wextra -Wunused
# make all warnings into errors
#CXXFLAGS+=-Werror
# optimize!
#CXXFLAGS+=-O3
# or not!
CXXFLAGS+=-O0
#
# das linker flags
# LDFLAGS+=
#
CXX:=g++
#
.SUFFIXES:
.SUFFIXES: .cpp .o
#
SHELL=/bin/sh
#
VERSION="$(shell cat VERSION)"
CXXFLAGS+=-DVERSION='$(VERSION)'
#
.PHONY: all clean

all: xboxfs

xboxfs: xboxfs.o xboxfs-code.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DPROGNAME='"$@"' -o $@ $^

%.o : %.cpp xboxfs.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DPROGNAME='"$@"' -o $@ -c $<

clean:
	-rm -f xboxfs *.o
