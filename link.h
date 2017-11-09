#ifndef LINK_H
#define LINK_H
#include <string>
#include <map>
#include "node.h"

using namespace std;

struct Each_link {
   int fd;
};

struct addr {
   addr(string _ip, int _port) : ip(_ip), port(_port) {}
   string ip;
   int port;
};

class Link {
public:
   Link(string ip, int port);
   void boot();
   void listen();
   void poll(Node *node);
   int find(string, int);
   int addNewNode(string ip, int port);
   
   bool connect(string &ip, int port);
   Each_link links[10240];

private:
   int port;
   string ip;
   int listenfd;

   map<addr, int> links_map;

   bool newConnection();
   void processNetwork(); 

};

#endif 
