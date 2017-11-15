#include <stdio.h>
#include "node.h"
#include <stdlib.h>
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
   int port = atoi(argv[3]);
   char *ip = argv[4];
   Node *serv;
   if (role == 0) {
      serv = new Node(role, "localhost", local_p, ip, port);
   } else {
      serv = new Node(role, "localhost", local_p, "localhost", local_p);
   }
   serv->init();
   serv->boot();

   delete serv; 
}
