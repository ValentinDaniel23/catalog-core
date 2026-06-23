CXX      ?= g++
CXXFLAGS  = -fPIE -Wall -std=c++17
LDFLAGS   = -static-pie

# Install: sudo apt install libleveldb-dev libsnappy-dev
LIBS = -lleveldb -lsnappy -lpthread -lm

.PHONY: all clean

all: hello-leveldb

hello-leveldb: hello.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	-rm -f hello-leveldb
