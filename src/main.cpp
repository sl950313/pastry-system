#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#define SERVER 1

void Usage() {
   printf("./pastry role local_port [ip] [port]\n");
}

int main(int argc, char **argv) {
   if (argc != 3 && argc != 5) {
      Usage();
      return -1;
   }
   int role = atoi(argv[1]);
   int local_p = atoi(argv[2]);
   int port;
   char *ip;
   if (argc == 5) {
      port = atoi(argv[4]);
      ip = argv[3];
   }
   Node *serv;
   if (role == 0) {
      serv = new Node(role, "127.0.0.1", local_p, ip, port);
   } else {
      serv = new Node(role, "127.0.0.1", local_p, "127.0.0.1", local_p);
   }
   serv->init();
   serv->boot();

   delete serv; 
}
