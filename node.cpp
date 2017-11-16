#include "node.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include "sha1.hpp"

#define BIT_NUM 4

#define START_PULL 1
//#define PUSH 0
#define FIRST_PUSH 2
#define FORWARD 4
#define FW_RESP 8
#define PULL 16
#define SYNC 32

Node::Node(int _role, string ip, int port, string servip, int servp) {
   role = (Role)_role;
   b = 4;
   l = 4;
   local_ip = ip;
   local_port = port;
   serv_ip = servip;
   serv_port = servp;
   id.ip = ip;
   id.port = port;

   /* for server */
   status = 0; // PUSH_LOOKUP_NODE.
}

Node::~Node() {
   delete links;
   for (int i = 0; i < BIT_NUM; ++i) {
      for (int j = 0; j < b; ++j)
         delete rtable[i][j];
      free(rtable[i]);
   }
   free(rtable);
}

void Node::init() {
   char tmp[256] = {0};
   sprintf(tmp, "%s:%d", local_ip.c_str(), local_port);
   string t = tmp;
   ID::makeID(t, &id);

   links = new Link(local_ip, local_port);
   links->listen();

   leaf_set.resize(l);
   rtable = (ID ***)malloc(sizeof(ID **) * BIT_NUM);
   for (int i = 0; i < BIT_NUM; ++i) {
      //rtable[i] = new ID();
      rtable[i] = (ID **)malloc(sizeof(ID) * id.b);
      for (int j = 0; j < b; ++j) {
         rtable[i][j] = new ID();
      }
      rtable[i][id.position(i)]->copy(id);// = id;
   }
   nset.resize(l);

   if (role == Client) {
      bool res = links->connect(serv_ip, serv_port);
      //assert(res, "error connect server[%s:%d]");
      if (!res) {
         printf("error connect server[%s:%d]\n", serv_ip.c_str(), serv_port);
         exit(-1);
      }
      printf("connect server[%s:%d] success.\n", serv_ip.c_str(), serv_port);
      // SYNC op.
      ID *tmp = new ID();
      tmp->ip = serv_ip;
      tmp->port = serv_port;
      int fd = getFdByNodeId(tmp);
      send(fd, SYNC, &id, NULL, 0);
      delete tmp;
   }

   /* boot heartbeat thread. */
}

void Node::newNode(Each_link *el) {
}

void *test(void *arg) {
   Node *node = (Node *)arg;
   int c = 10;
   while (c--) {
      getchar(); 
      ID *key = (ID *)malloc(sizeof(ID));
      char msg[32] = {0};
      sprintf(msg, "Hello World %d!", rand() % 1024);
      string t = msg;
      ID::makeID(t, key);
      node->push(key, msg);
      free(key);
   }
   return NULL;
}

void Node::boot() {
   pthread_t pid;
   pthread_create(&pid, NULL, test, this);
   links->poll(this);
}

void Node::processNetworkMsg(char *buf, int len) { 
   printf("recv buf : [%s]\n", buf);
   unuse(&len);
   char op = 0x00;// = buf[0];
   //char *data = buf + 1;
   ID *src = new ID();
   char *extra_msg = NULL;
   decode(buf, op, src, extra_msg); 

   switch (op) {
   case START_PULL:
      {
         //ID *key = (ID *)malloc(sizeof(*key));
         printf("recv a START_PULL op [%s:%d]\n", src->ip.c_str(), src->port);
         ID *key = new ID();
         memcpy(&key->id, extra_msg, sizeof(int));
         //key->id = (int)extra_msg;
         key->ip = local_ip;
         key->port = local_port;
         //pull(key);
         int fd = lookup(key);
         if (fd == 0) {
            // the source of key is localed.
            // nothing to do.
         } else if (fd > 0) {
            key->id = id.id;
            send(fd, FORWARD, key, (char *)&key->id, 4);
            // the source is localed on node with filescription fd.
         } else {
            // wait for FW_RESP.
         }
         delete key;
         break;
      }
   case FIRST_PUSH:
      printf("recv a FIRST_PUSH op [%s:%d]\n", src->ip.c_str(), src->port);
      saveMsg(extra_msg,  strlen(extra_msg));
      break;
   case FORWARD:
      {
         printf("recv a FORWARD op [%s:%d]\n", src->ip.c_str(), src->port);
         //ID *key = (ID *)malloc(sizeof(*key));
         ID *key = new ID();
         memcpy(&key->id, extra_msg, sizeof(int));
         //key->id = (int)extra_msg;
         key->ip = src->ip;
         key->port = src->port;
         int fd = lookup(key);
         if (fd == 0) {
            fd = getFdByNodeId(key);
            //string msg = local_ip + to_string(local_port);
            send(fd, FW_RESP, &id, NULL, 0);
         } else if (fd > 0) {
            //char msg[64] = {0};
            //sprintf(msg, "%d%s%d", (int)key->ip.length(), key->ip.c_str(), key->port);
            send(fd, FORWARD, src, (char *)&key->id, 4);
         } else {
         }
         delete key;
         break;
      }
   case FW_RESP:
      {
         printf("recv a FW_RESP op [%s:%d]\n", src->ip.c_str(), src->port);
         if (status == 1) { // means the RESP is received by server.
            push(src, NULL);
         } else { // the RESP is received by client.
            // TODO: some other strategy should be added.
            saveMsg(extra_msg, strlen(extra_msg));
         }
         break;
      }
   case SYNC:
      {
         printf("recv a SYNC op [%s:%d]\n", src->ip.c_str(), src->port);
         break;
      }
   default:
      printf("pastry recerive other msg[%s], just print!\n", buf);
      break;
   }
   delete src;
}

/*
 * server run the command.
 */
void Node::push(ID *key, char *message) {
   /* let the message to the right node-id*/
   switch (status) {
   case 0:
      {
         int fd = lookup(key);
         status = 1;
         if (fd > 0) {
            //status = 0;
            send(fd, FIRST_PUSH, &id, NULL, 0);
         } else if (fd == 0) {
            /* the key should be localed.*/
            saveMsg(message, strlen(message));
         } else {
            break;
            // wait for msg receive from other node with op FW_RESP.
         }
      }
   case 1:
      {
         for (int i = 0; i < 10240; ++i) {
            if (links->links[i].fd == 0) continue;
            send(links->links[i].fd, START_PULL, &id, (char *)&key->id, sizeof(key->id));
         }
         status = 0;
      }
      break;
   default:
      break;
   }
}

void Node::pull(ID *key) {
   lookup(key);
}

/*
 * core route algorithm of pastry
 */
void Node::route(ID *key) {
   int p = id.sha_pref(key);
   int v = key->position(p - 1);

   if (rtable[p][v]->notNull()) {
      forward(rtable[p][v], key);
   } else {
      int min_dis = INT_MAX;
      int pos = -1;
      for (int i = 0; i < BIT_NUM; ++i) {
         if (!rtable[p][i]->notNull()) {
            int dis = rtable[p][i]->distance(key);
            if (dis < min_dis) {
               pos = i;
               min_dis = dis;
            }
         }
      }
      forward(rtable[p][pos], key);
   }
}

int Node::lookupKey(ID *key) {
   /* check whether the key is in the leaf set */
   if (key->bigger(&leaf_set[0]) && key->bigger(&leaf_set[leaf_set.size() - 1])) {
      int closest = -1;
      int min_dis = INT_MAX;
      for (size_t i = 0; i < leaf_set.size(); ++i) {
         int dis = key->distance(&leaf_set[i]);
         if (dis < min_dis) {
            closest = i;
            min_dis = dis;
         }
      }
      if (leaf_set[closest].addrEqual(&id)) {

      }
      forward(&leaf_set[closest], key);
      return -1;
   } 
}

int Node::lookup(ID *key, bool server) { 
   unuse(&server);
   /* check whether the key is controled by myself. */
   map<ID *, string >::iterator it = keys.find(key);
   int fd = -1;
   if (it != keys.end()) {
      if (key->ip.compare(local_ip) && key->port == local_port) {
         fd = 0;
         // the key is localed.
      } else {
         fd = links->find(key->ip, key->port);
         if (fd == -1) {
            printf("The reason goes here, key[ip:%s,port%d,id:%d]\n", key->ip.c_str(), key->port, key->id);
            //links->addNewNode(key->ip, key->port);
            //fd = links->find(key->ip, key->port);
         }
      }
      return fd;
   }

   /* check whether the key is in the leaf set */
   if (key->bigger(&leaf_set[0]) && key->bigger(&leaf_set[leaf_set.size() - 1])) {
      int closest = -1;
      int min_dis = INT_MAX;
      for (size_t i = 0; i < leaf_set.size(); ++i) {
         int dis = key->distance(&leaf_set[i]);
         if (dis < min_dis) {
            closest = i;
            min_dis = dis;
         }
      }
      forward(&leaf_set[closest], key);
      return -1;
   }

   /* find the closest node from route table. */
   route(key);
   return -1;
}

void Node::forward(ID *id, ID *key) {
   int fd = links->find(id->ip, id->port);
   char msg[64] = {0};
   int len = 1 + 4 + key->ip.length() + 4;
   sprintf(msg, "%d%c%d%s%d%d", len, FORWARD, (int)key->ip.length(), key->ip.c_str(), key->port, key->id);
   write(fd, msg, len + 4);
}

void Node::nodesDiscoveryAlg() {
}

void Node::saveMsg(char *msg, int len) {
   unuse(&len);
   string _msg = msg;
   ID *key = (ID *)malloc(sizeof(*key));
   ID::makeID(_msg, key);
   keys.insert(pair<ID *,  string >(key, _msg));
} 

void Node::send(ID *des, char op, ID *src, char *message, int msg_len) {
   int fd = getFdByNodeId(des);
   send(fd, op, src, message, msg_len);
}

void Node::send(int fd, char op, ID *keyorId, char *msg, int msg_len) {
   char _msg[64] = {0};
   int len = 1 + 1 + keyorId->ip.length() + 4 + 4 + 4 + msg_len;
   sprintf(_msg, "%4d%c%c%s%4d%4d%4d%s", len, op, (int)keyorId->ip.length(), keyorId->ip.c_str(), keyorId->port, keyorId->id, msg_len, msg);
   int nwrite = write(fd, _msg, len + 4); 
   printf("nwrite = [%d:%s],len=[%d],op=[%d]\n", nwrite, _msg, len, op);
}

void Node::encode(char op, ID *src, char *extra_msg, int msg_len) {
}

void Node::decode(char *data, char &op, ID *target, char *extra_msg) {
   op = data[0];
   data++;
   int ip_len = data[0];
   data++;
   char ip[32] = {0};
   memcpy(ip, data, ip_len);
   target->ip = ip;
   data += ip_len;
   char tmp[8] = {0};
   memcpy(tmp, data, 4);
   target->port = atoi(tmp);
   data += 4;
   memcpy(tmp, data, 4);
   target->id = atoi(tmp);
   data += 4;
   //int msg_len = ((int *)data)[0];
   data += 4;
   extra_msg = data;
   /*
   switch (op) {
   case FORWARD:
      { 
         int len = 0;
         memcpy(&len, data, 4);
         char ip[32] = {0};
         memcpy(ip, data + 4, len);
         target->ip = ip;
         memcpy(&target->port, data + 4 + len, 4);
         memcpy(&target->id, data + 4 + len + 4, 4);
         break;
      }
   default:
      break;
   }
   */
}

int Node::getFdByNodeId(ID *id) {
   return links->find(id->ip, id->port);
}

void Node::assert(bool assert, char *str) {
   if (!assert) {
      printf("%s\n", str);
      exit(-1);
   }
}

void unuse(void *) {}
