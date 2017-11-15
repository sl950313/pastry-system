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
#define REMAIN 10

void Link::listen() {
   int    socket_fd;
   if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
      printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
      exit(0);  
   } 
   struct sockaddr_in     servaddr;  
   memset(&servaddr, 0, sizeof(servaddr));  
   servaddr.sin_family = AF_INET;  
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons(port);//设置的端口为DEFAULT_PORT  

   if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
      printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
      exit(0);  
   }

   if(::listen(socket_fd, 10) == -1){  
      printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
      exit(0);  
   }  
   printf("======waiting for client's request======\n");  
   listenfd = socket_fd;
}

void Link::poll(Node *node) {
   epfd = epoll_create(1);
   struct epoll_event ev,events[10240];

   ev.data.fd = listenfd;
   ev.events = EPOLLIN;

   epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

   while (true) {
      int nfds = epoll_wait(epfd, events, MAX_NODE, 1000);

      if (node->role == 1) { /* Server need more work. */
         /*
         ID *key = (ID *)malloc(sizeof(ID));
         char msg[32] = {0};
         sprintf(msg, "Hello World %d!", rand() % 1024);
         string t = msg;
         ID::makeID(t, key);
         node->push(key, msg);
         free(key);
         */
      }

      for (int i = 0; i < nfds; ++i) {
         if (events[i].data.fd == listenfd) {
            newConnection(node);
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

bool Link::newConnection(Node *node) {
   struct sockaddr_in conn_addr;
   socklen_t addrlen = sizeof(conn_addr);
   int connfd = accept(listenfd, (struct sockaddr *)&conn_addr, &addrlen);
   struct epoll_event ev;
   ev.data.fd = connfd;

   epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

   string ip = inet_ntoa(conn_addr.sin_addr);
   int port = ntohs(conn_addr.sin_port);
   links_map.insert(pair<addr, int>(addr(ip, port), connfd));

   Each_link el(connfd, ip, port);
   links[connfd] = el;

   node->newNode(&el);

   return true;
}

int Link::find(string ip, int port) {
   map<addr, int>::iterator it = links_map.find(addr(ip, port));
   if (it != links_map.end()) {
      return it->second;
   } else {
      printf("error occure in Link::find[ip:%s,port:%d]\n", ip.c_str(), port);
      return -1;
   }
}

bool Link::connect(string &ip, int port) {
   int client_sockfd;  
   struct sockaddr_in remote_addr;
   memset(&remote_addr,0,sizeof(remote_addr));
   remote_addr.sin_family=AF_INET;
   remote_addr.sin_addr.s_addr=inet_addr(ip.c_str());
   remote_addr.sin_port=htons(port);
   if ((client_sockfd=socket(PF_INET,SOCK_STREAM,0)) < 0)  {  
      return false;  
   }

   if (::connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr)) < 0) {  
      //perror("connect");  
      return false;  
   } 

   links[client_sockfd].ip = ip;
   links[client_sockfd].port = port;
   links[client_sockfd].fd = client_sockfd;
   return true;
}
