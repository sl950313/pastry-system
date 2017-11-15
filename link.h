#ifndef LINK_H
#define LINK_H
#include <string>
#include <map>
#include "node.h"

using namespace std;

struct Each_link {
   int fd;
   string ip;
   int port;

   Each_link() {fd = -1;}
   Each_link(int _fd, string &_ip, int _p) : fd(_fd), ip(_ip), port(_p) {}
};

struct addr {
   addr(string _ip, int _port) : ip(_ip), port(_port) {}
   string ip;
   int port;

   bool operator<(addr other) const {
      return ip.compare(other.ip) < 0 && port < other.port;//>>
   }

};

class Node;

class Link {
public:
   Link(string _ip, int _port) {ip = _ip;port = _port;}
   void boot();
   void listen();
   void poll(Node *node);
   int find(string, int);
   
   bool connect(string &ip, int port);
   Each_link links[10240];

private:
   int port;
   string ip;
   int listenfd;
   int epfd;

   map<addr, int> links_map;

   bool newConnection(Node *node);
   void processNetwork(); 

};

#endif 
