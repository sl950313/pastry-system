all: pastry


pastry: node.cpp node.h link.cpp link.h
	g++ -o pastry node.cpp main.cpp link.cpp link.h sha1.hpp sha1.cpp -lpthread -std=c++11 -g

clean:
	rm pastry
