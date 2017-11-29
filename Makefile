LIBS=-lglog -lgtest -lpthread
FLAGS=-std=c++11
DEBUG=-g
CC=g++

all: pastry gtest

pastry: node.cpp node.h link.cpp link.h defines.h id.h main.cpp
	$(CC) -o pastry node.cpp main.cpp link.cpp link.h sha1.hpp sha1.cpp $(FLAGS) $(DEBUG) $(LIBS)

gtest: test.cpp node.h id.h test-id.cpp
	$(CC) -o gtest test.cpp test-id.cpp $(FLAGS) $(LIBS) $(DEBUG)

clean:
	rm pastry gtest
