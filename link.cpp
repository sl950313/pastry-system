#include "link.h"
#include "node.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_NODE 10240

void Link::poll(Node *node) {
   int epfd = epoll_create(1);
   struct epoll_event ev,events[10240];

   ev.data.fd = listenfd;
   ev.events = EPOLLIN;

   epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

   while (true) {
      int nfds = epoll_wait(epfd, events, MAX_NODE, 1000);

      if (node->role == 1) { /* Server need more work. */
         ID *key = (ID *)malloc(sizeof(ID));
         char msg[32] = {0};
         sprintf(msg, "Hello World %d!", rand() % 1024);
         string t = msg;
         ID::makeID(t, key);
         node->push(key, msg);
         free(key);
      }

      for (int i = 0; i < nfds; ++i) {
         if (events[i].data.fd == listenfd) {
            newConnection();
         } else if (events[i].events & EPOLLIN) { 
            int len = -1;
            char buf[256] = {0};
            int nread = read(events[i].data.fd, &len, sizeof(len));
            if (nread != sizeof(len) || len == -1) {
               printf("error ocuur\n");
               continue;
            } else {
               memset(buf, 0, 256);
               nread = read(events[i].data.fd, buf, len);
               if (nread != len) {
                  printf("error ocuur\n"); 
               } else {
                  node->processNetworkMsg(buf, nread);
               }
            }
         }
      }
   }
}
