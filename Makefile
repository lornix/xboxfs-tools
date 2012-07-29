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
CXXPRG=xboxfs
#
.PHONY: all clean

all: $(CXXPRG)

% : %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DPROGNAME='"$@"' -o $@ $<

clean:
	-rm -f $(CXXPRG)
