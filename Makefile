LIBS=-lpthread -lglog
all: pastry

pastry: node.cpp node.h link.cpp link.h defines.h
	g++ -o pastry node.cpp main.cpp link.cpp link.h sha1.hpp sha1.cpp -std=c++11 -g $(LIBS)

clean:
	rm pastry
