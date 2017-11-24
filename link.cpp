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
#include <glog/logging.h>

#define MAX_NODE 10240
#define REMAIN 10

void Link::init() {
   epfd = epoll_create(1);
}

void Link::listen() {
   int    socket_fd;
   if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
      LOG(ERROR) << "create socket error:" << strerror(errno) << "(errno:" << errno << ").";
      printf("create socket error\n");
#ifdef DEBUG
      google::FlushLogFiles(google::INFO);
#endif
      exit(0);  
   } 
   struct sockaddr_in     servaddr;  
   memset(&servaddr, 0, sizeof(servaddr));  
   servaddr.sin_family = AF_INET;  
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons(port);

   if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
      LOG(ERROR) << "bind socket error:" << strerror(errno) << "(errno:" << errno << ").";
      printf("bind socket error\n");
#ifdef DEBUG
      google::FlushLogFiles(google::INFO);
#endif
      exit(0);  
   }

   if(::listen(socket_fd, 10) == -1){  
      LOG(ERROR) << "listen socket error: " << strerror(errno) << ":" << errno;
      printf("listen socket error\n");
#ifdef DEBUG
      google::FlushLogFiles(google::INFO);
#endif
      exit(0);  
   }  
   LOG(INFO) << "======waiting for client's request======";
   printf("======waiting for client's request======\n");
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif
   listenfd = socket_fd;
}

void Link::poll(Node *node) {
   struct epoll_event ev,events[10240];

   ev.data.fd = listenfd;
   ev.events = EPOLLIN;

   epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);

   while (true) {
      int nfds = epoll_wait(epfd, events, MAX_NODE, 1000);

      if (node->role == 1) { /* Server need more work. */
      }

      if (nfds > 0) {
         printf("%d fd events occure\n", nfds);
         LOG(INFO) << nfds << " fd events occure";
#ifdef DEBUG
         google::FlushLogFiles(google::INFO);
#endif
      }
      for (int i = 0; i < nfds; ++i) {
         if (events[i].data.fd == listenfd) {
            newConnection(node);
         } else if (events[i].events & EPOLLIN) { 
            char buf[256] = {0};
            if (!links[events[i].data.fd].use) {
               int port = -1;
               int fd = events[i].data.fd;
               read(events[i].data.fd, &port, sizeof(int)); 
               LOG(INFO) << "receive port : " << port;
               printf("receive port : %d\n", port);
#ifdef DEBUG
               google::FlushLogFiles(google::INFO);
#endif
               links[events[i].data.fd].port = port;
               links[events[i].data.fd].use = true;
               links_map.insert(pair<addr, int>(addr(links[events[i].data.fd].ip, port), fd));
               node->newNode(&links[events[i].data.fd]);
               continue;
            }
            int nread = read(events[i].data.fd, buf, sizeof(int));
            int len = atoi(buf);
            LOG(INFO) << "nread = [" << nread << ", len = [" << len << "].";
            printf("nread = [%d], len = [%d]\n", nread, len);
#ifdef DEBUG
            google::FlushLogFiles(google::INFO);
#endif
            memset(buf, 0, 4);
            if (nread == 0) {
               cleaningWork(events[i].data.fd);
               continue;
            }
            if (nread != sizeof(len) && len == -1) {
               LOG(INFO) << "error ocuur";
               printf("error occure\n");
#ifdef DEBUG
               google::FlushLogFiles(google::INFO);
#endif
               continue;
            } else {
               memset(buf, 0, 256);
               nread = read(events[i].data.fd, buf, len);
               LOG(INFO) << "nread=[" << nread << "],len=[" << len << "],buf=[" << buf << "].";
               printf("nread = [%d], len = [%d], buf=[%s]\n", nread, len, buf);
#ifdef DEBUG
               google::FlushLogFiles(google::INFO);
#endif
               if (nread != len) {
                  LOG(INFO) << "error ocuur"; 
                  printf("error occure\n");
#ifdef DEBUG
                  google::FlushLogFiles(google::INFO);
#endif
               } else {
                  node->processNetworkMsg(buf, nread);
               }
            }
         }
      }
   }
}

void Link::cleaningWork(int fd) {
   epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
   close(fd);
   links_map.erase(links_map.find(addr(links[fd].ip, links[fd].port)));
   links[fd].use = false;
   printf("node [%s:%d] currently disconnected\n", links[fd].ip.c_str(), links[fd].port);
}

bool Link::newConnection(Node *node) {
   unuse(node);
   struct sockaddr_in conn_addr;
   socklen_t addrlen = sizeof(conn_addr);
   int connfd = accept(listenfd, (struct sockaddr *)&conn_addr, &addrlen);
   struct epoll_event ev;
   ev.data.fd = connfd;
   ev.events = EPOLLIN;

   epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

   string ip = inet_ntoa(conn_addr.sin_addr);
   int port = ntohs(conn_addr.sin_port);

   Each_link el(connfd, ip, port);
   links[connfd] = el;
   LOG(INFO) << "A new connection : [" << el.ip << ":" << el.port << "].";
   printf("A new connection : [%s:%d]\n", el.ip.c_str(), el.port);
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif


   return true;
}

int Link::find(string ip, int port) {
   map<addr, int>::iterator it = links_map.find(addr(ip, port));
   if (it != links_map.end()) {
      return it->second;
   } else {
      LOG(ERROR) << "error occure in Link::find[ip:" << ip << ",port:" << port << "].";
      printf("error occure in Link::find[ip:%s,port:%d]\n", ip.c_str(), port);
#ifdef DEBUG
      google::FlushLogFiles(google::INFO);
#endif
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
      perror("connect");  
      return false;  
   } 

   links[client_sockfd].ip = ip;
   links[client_sockfd].port = port;
   links[client_sockfd].fd = client_sockfd;
   links[client_sockfd].use = true;
   links_map.insert(pair<addr, int>(addr(ip, port), client_sockfd));
   struct epoll_event ev;
   ev.data.fd = client_sockfd;
   ev.events = EPOLLIN;

   epoll_ctl(epfd, EPOLL_CTL_ADD, client_sockfd, &ev);
   return true;
}
