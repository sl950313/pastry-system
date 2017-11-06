#include "node.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include "sha1.hpp"

#define BIT_NUM 8

#define PULL 1
#define PUSH 0
#define FIRST_PUSH 2
#define FORWARD 3

Node::Node(int _role, string ip, int port, string servip, int servp) {
   role = (Role)_role;
   b = 2;
   l = 8;
   local_ip = ip;
   local_port = port;
   serv_ip = servip;
   serv_port = servp;

   /*
   SHA1 checksum;
   checksum.update(ip);
   memcpy(id, checksum.final().c_str(), 16);
   */
}

Node::~Node() {
   delete links;
   for (int i = 0; i < BIT_NUM; ++i)
      free(rtable[i]);
   free(rtable);
}

void Node::init() {
   links = new Link(local_ip, local_port);
   links->listen();

   leaf_set.resize(l);
   rtable = (ID **)malloc(sizeof(int *) * BIT_NUM);
   for (int i = 0; i < BIT_NUM; ++i) {
      rtable[i] = (ID *)malloc(sizeof(int) * b);
      memset(rtable[i], 0, sizeof(int) * b);
   }
   nset.resize(l);

   char tmp[256] = {0};
   sprintf(tmp, "%s:%d", local_ip.c_str(), local_port);
   string t = tmp;
   ID::makeID(t, &id);

   if (role == Client) {
      bool res = links->connect(serv_ip, serv_port);
      if (!res) {
         printf("error connect server[%s:%d]\n", serv_ip.c_str(), serv_port);
         exit(-1);
      }
   }

   /* boot heartbeat thread. */
}

void Node::boot() {
   links->poll(this);
}

void Node::processNetworkMsg(char *buf, int len) { 
   unuse(&len);
   char op = buf[0];
   char *data = buf + 1;
   switch (op) {
   case PULL:
      {
      ID *key = (ID *)malloc(sizeof(*key));
      key->id = (int)data;
      key->ip = local_ip;
      key->port = local_port;
      pull(key);
      free(key);
      break;
      }
   case FIRST_PUSH:
      saveMsg(data, len - 1);
      break;
   default:
      printf("pastry recerive other msg[%s], just print!\n", buf);
      break;
   }
}

void Node::saveMsg(char *msg, int len) {
   unuse(&len);
   string _msg = msg;
   ID *key = (ID *)malloc(sizeof(*key));
   ID::makeID(_msg, key);
   keys.insert(pair<ID *,  string >(key, _msg));
}

void Node::push(ID *key, void *message) {
   /* let the message to the right node-id*/
   ID node_id = getNodeByKey(key);
   char msg[64] = {0};
   int len = 4 + 1 + 4;
   char op = FIRST_PUSH;
   //int data = key->id;

   memcpy(msg, &len, 4);
   memcpy(msg + 4, &op, 1);
   memcpy(msg + 5, message, ((int *)message)[0]);
   write(getFdByNodeId(&node_id), msg, len);

   /* notify other nodes to pull the message */
   for (int i = 0; i < 10240; ++i) {
      if (links->links[i].fd == 0) continue;
      memset(msg, 0, 64);
      op = PULL;
      int data = key->id;
      memcpy(msg, &len, 4);
      memcpy(msg + 4, &op, 1);
      memcpy(msg + 5, &data, 4);
      write(links->links[i].fd, msg, len);
   }
   //route(message, key);
}

void Node::pull(ID *key) {
   lookup(key);
}

/*
 * core route algorithm of pastry
 */
void Node::route(ID *key) {
   int p = sha_pref(key);
   int v = key->position(p);

   if (rtable[p][v].notNull(p, v)) {
      forward(&rtable[p][v], key);
   } else {
      int min_dis = INT_MAX;
      int pos = -1;
      for (int i = 0; i < BIT_NUM; ++i) {
         if (!rtable[p][i].notNull(p, i)) {
            int dis = rtable[p][i].distance(key);
            if (dis < min_dis) {
               pos = i;
               min_dis = dis;
            }
         }
      }
      forward(&rtable[p][pos], key);
   }
}

int Node::sha_pref(ID *key) {
   unuse(key);
   return -1;
}

int Node::lookup(ID *key) { 
   /* check whether the key is controled by myself. */
   map<ID *, string >::iterator it = keys.find(key);
   if (it != keys.end()) {
      if (key->ip.compare(local_ip) && key->port == local_port) {
         // the key is localed.
      } else {
         int fd = links->find(pair<string, int>(key->ip, key->port));
         if (fd == -1) {
            links->addNewNode(key->ip, key->port);
            fd = links->find(pair<string, int>(key->ip, key->port));
         }
         write(fd, it->second.c_str(), it->second.length());
      }
      return 1;
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
      return 0;
   }

   /* find the closest node from route table. */
   route(key);
   return 0;
}

void Node::forward(ID *id, ID *key) {
   int fd = links->find(pair<string, int>(id->ip, id->port));
   //string msg = makeMsg(id, key, PULL);
   char msg[64] = {0};
   write(fd, msg.c_str(), msg.length());
}

void Node::makeMsg(ID *id, ID *key, char op) {
   unuse(id);
   unuse(key);
   char msg[64] = {0};
   int len = 4 + 1 + 4;
   //int data = key->id;

   memcpy(msg, &len, 4);
   memcpy(msg + 4, &op, 1);
   //memcpy(msg + 5, &key->id, );
}
