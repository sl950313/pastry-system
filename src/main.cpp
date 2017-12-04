#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"

#define SERVER 1

void Usage() {
    printf("./pastry role local_port [ip] [port]\n");
}

void user_interface(Node *serv) {
    bool running = true;
    while (running) {
        printf("What message would you want to send? [q to quit our system]\n");
        char buf[BUF_LEN];
        fgets(buf, BUF_LEN, stdin);
        int len = strlen(buf);
        buf[len - 1] = '\0';
        if (buf[0] == 'q' && strlen(buf) == 1) {
            running = false;
            break;
        }
        serv->push(buf, strlen(buf));
    }
    printf("Welcome Back Our System.\n");
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

    user_interface(serv);
    
    delete serv; 
}
