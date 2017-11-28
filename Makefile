LIBS=-lpthread -lglog
DEBUG=-g
CC=g++

all: pastry

pastry: node.cpp node.h link.cpp link.h defines.h id.h
	$(CC) -o pastry node.cpp main.cpp link.cpp link.h sha1.hpp sha1.cpp -std=c++11 $(DEBUG) $(LIBS)

clean:
	rm pastry
