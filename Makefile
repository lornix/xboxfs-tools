#
# pretty much always want debugging symbols included
CXXFLAGS+=-g
#CXXFLAGS+=-pg
# yell out all warnings and whatnot
CXXFLAGS+=-Wall -Wextra -Wunused
# make all warnings into errors
#CXXFLAGS+=-fprofile-arcs -ftest-coverage
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
#
PROGNAME=xboxfs

all: $(PROGNAME)

$(PROGNAME): xboxfs.o xboxfs-code.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DPROGNAME='"$(PROGNAME)"' -o $@ $^

%.o : %.cpp xboxfs.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DPROGNAME='"$(PROGNAME)"' -o $@ -c $<

clean:
	-rm -f xboxfs *.o
