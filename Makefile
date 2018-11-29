# Makefile for tommy's server.
CXX= g++
CXXFLAGS= -g -I/usr/include/lua5.2
LDFLAGS=-g -L/usr/lib/x86_64-linux-gnu -pthread
LINK.o = $(LINK.cc)

PROG=server
all: $(PROG)

$(PROG): $(PROG).o

$(PROG).o: $(PROG).c


.PHONY: clean run depend

clean:
	rm -f $(PROG) $(PROG).o

run: all
	./server.exe 10000

depend:
	touch make.depend
	$(CXX) -M $(CXXFLAGS) $(PROG).cpp > make.depend

-include make.depend
