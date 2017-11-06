#ifndef LINK_H
#define LINK_H
#include <string>
#include "node.h"

using namespace std;

struct Each_link {
   int fd;
};

class Link {
public:
   Link(string ip, int port);
   void boot();
   void listen();
   void poll(Node *node);
   int find(pair<string, int>);
   int addNewNode(string ip, int port);
   
   bool connect(string &ip, int port);
   Each_link links[10240];

private:
   int port;
   string ip;
   int listenfd;

   bool newConnection();
   void processNetwork(); 

};

#endif 
