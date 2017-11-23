#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include <glog/logging.h>
#include <sys/types.h>
#include "node.h"
#include "sha1.hpp"
#include "defines.h"

#define BIT_NUM 4

Node::Node(int _role, string ip, int port, string servip, int servp) {
   role = (Role)_role;
   b = 4;
   l = 4;
   right = 1;
   left = 0;
   s = l / 2;
   f = l / 2;
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
   for (int i = 0; i < l; ++i)
      delete leaf_set[i];
   delete leaf_set;
   for (int i = 0; i < BIT_NUM; ++i) {
      for (int j = 0; j < b; ++j)
         delete rtable[i][j];
      free(rtable[i]);
   }
   free(rtable);
}

void Node::init() {
   //daemonize();

   google::InitGoogleLogging("");
   google::SetLogDestination(google::INFO, LOG_FILENAME);

   LOG(INFO) << "Google Log System already initial"; 
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif

   char tmp[256] = {0};
   sprintf(tmp, "%s:%d", local_ip.c_str(), local_port);
   string t = tmp;
   ID::makeID(t, &id);

   links = new Link(local_ip, local_port);
   links->listen();

   leaf_set = (ID **)malloc(sizeof(ID *) * l);
   for (int i = 0; i < l; ++i)
      leaf_set[i] = new ID();
   leaf_set[l / 2 - 1]->copy(id);
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
         LOG(ERROR) << "error connect server[" << serv_ip << ":" << serv_port << "].";
#ifdef DEBUG
         google::FlushLogFiles(google::INFO);
#endif
         exit(-1);
      }
      LOG(INFO) << "connect server[" << serv_ip << ":" << serv_port << "].";
#ifdef DEBUG
      google::FlushLogFiles(google::INFO);
#endif
      // SYNC op.
      ID *tmp = new ID();
      tmp->ip = serv_ip;
      tmp->port = serv_port;
      int fd = getFdByNodeId(tmp);
      write(fd, &local_port, 4);
      send(fd, SYNC, &id, NULL, 0);
      status = 2;
      delete tmp;
   }

   /* boot heartbeat thread. */
}

void Node::newNode(Each_link *el) {
   updateLeafSetWithNewNode(el);
   updateRouteTableWithNewNode(el);
}

void Node::updateLeafSetWithNewNode(Each_link *el) {
   ID *t_id = new ID();
   t_id->ip = el->ip;
   t_id->port = el->port;
   ID::defaultMakeID(t_id);
   if (t_id->bigger(&id)) {
      for (int i = l / 2 + 1; i < l; ++i) {
         if (!leaf_set[i]->notNull()) {
            leaf_set[i]->copy(*t_id);
            f++;
         } else if (leaf_set[i]->bigger(t_id)) { 
            for (int j = i + 1; j < l; ++j) {
               leaf_set[j]->copy(*leaf_set[j - 1]);
            }
            leaf_set[i]->copy(*t_id);
            f++;
         }
      }
   } else {
      for (int i = l / 2 - 1; i >= 0; --i) {
         if (!leaf_set[i]->notNull()) {
            leaf_set[i]->copy(*t_id);
            s--;
         } else if (!leaf_set[i]->bigger(t_id)) { 
            for (int j = i - 1; j >= 0; --j) {
               leaf_set[j]->copy(*leaf_set[j + 1]);
            }
            leaf_set[i]->copy(*t_id);
            s--;
         }
      }
   }
   delete t_id;
}

void Node::updateRouteTableWithNewNode(Each_link *el) { 
   ID *t_id = new ID();
   t_id->ip = el->ip;
   t_id->port = el->port;
   ID::defaultMakeID(t_id); 
   int p = id.sha_pref(t_id);
   int v = t_id->position(p);

   if (!rtable[p][v]->notNull()) {
      rtable[p][v]->copy(*t_id);
   }
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
   //pthread_t pid;
   //pthread_create(&pid, NULL, test, this);
   links->poll(this);
}

void Node::processNetworkMsg(char *buf, int len) { 
   LOG(INFO) << "recv buf : [" << buf << "].";
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif
   unuse(&len);
   char op = 0x00;// = buf[0];
   //char *data = buf + 1;
   ID *src = new ID();
   char *extra_msg = NULL;
   decode(buf, op, src, extra_msg); 

   LOG(INFO) << "recv a " << opToStr(op) << ":[" << src->ip << ":" << src->port << "].";
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif
   switch (op) {
   case START_PULL:
      {
         //ID *key = (ID *)malloc(sizeof(*key));
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
      saveMsg(extra_msg,  strlen(extra_msg));
      break;
   case FORWARD:
      {
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
         if (status == 1) { // means the RESP is received by server.
            push(src, NULL);
         } else { // the RESP is received by client.
            // TODO: some other strategy should be added.
            if (status == 0) 
               saveMsg(extra_msg, strlen(extra_msg));
            else if (status == 2){ // means the RESP is a SYNC info.
               send(getFdByNodeId(src), SYNC_RT, &id, NULL, 0);
            }
         }
         break;
      }

   case SYNC:
      {
         ID *node = lookupKey(src);
         LOG(INFO) << "after lookupKey, the ret node : [ip:" << node->ip << ",port:" << node->port << "].";
#ifdef DEBUG
         google::FlushLogFiles(google::INFO);
#endif
         CHECK_NOTNULL(node);
         if (node->addrEqual(&id)) {
            char msg[1024] = {0};
            serializeLeafSetAndRouteTable(msg);
            send(src, SYNC_RT, &id, msg, strlen(msg));
         } else {
            send(node, SYNC, src, NULL, 0);
         }
         break;
      }
   case SYNC_RT:
      {
         deserializeLeafSetAndRouteTable(extra_msg);
         break;
      }
   default:
      break;
   }
   delete src;
}

void Node::deserializeLeafSetAndRouteTable(char *msg) {
   LOG(INFO) << "Updating Route Table and Leaf set.";
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif
   int offset = 0;
   for (int i = 0; i < l; ++i) {
      offset = deserialize(msg + offset, leaf_set[i]);
   }
   correctLeafSet();

   for (int i = 0; i < BIT_NUM; ++i)
      for (int j = 0; j < b; ++j)
         offset = deserialize(msg + offset, rtable[i][j]); 
   correctRouteTable(); 
}

void Node::serializeLeafSetAndRouteTable(char *msg) {
   int offset = 0;
   for (int i = 0; i < l; ++i) {
      offset = serialize(leaf_set[i], msg + offset);
   }

   for (int i = 0; i < BIT_NUM; ++i) {
      for (int j = 0; j < b; ++i) {
         offset = serialize(rtable[i][j], msg + offset);
      }
   }
   LOG(INFO) << "SYNC rtable and leaf_set total len : " << offset;
}

void Node::correctRouteTable() {
   for (int i = 0; i < BIT_NUM; ++i) {
      int index = id.position(i);
      rtable[i][index]->copy(id);
   }
}

void Node::correctLeafSet() { 
   leaf_set[l / 2 - 1]->copy(id);
   int index = -1;
   for (int i = 0; i < l / 2 - 1; ++i) {
      if (leaf_set[i]->id >= id.id) {
         index = i;
         break;
      }
   }
   if (index != -1) {
      for (int i = index; i >= 0; --i) {
         leaf_set[l / 2 - 1 - index + i]->copy(*leaf_set[i]);
      }
      for (int i = 0; i < l / 2 - 1 - index; ++i) {
         leaf_set[i]->id = -1;
      } 
   }
   index = -1; 
   for (int i = 1; i < l / 2 - 1; ++i) {
      if (leaf_set[l / 2 + i]->id > id.id) {
         index = l / 2 + i;
         break;
      }
   }
   if (index != -1 && index != 1) {
      for (int i = l / 2 + 1; i < l - index + l / 2; ++i) {
         leaf_set[i]->copy(*leaf_set[i - l / 2 - 1 + index]);
      }
      for (int i = l - index + l / 2; i < l; ++i) {
         leaf_set[i]->id = -1;
      }
   }
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
   int fd = lookup(key);
}

ID *Node::lookupLeafSet(ID *key) {
   for (int i = 0; i < l; ++i) {
      if (leaf_set[i]->notNull()) {
         s = i;
         break;
      }
   }
   for (int i = l - 1; i >= 0; --i) {
      if (leaf_set[i]->notNull()) {
         f = i;
         break;
      }
   }
   if (key->bigger(leaf_set[s]) && !key->bigger(leaf_set[f])) {
      ID *closest = NULL;
      int min_dis = INT_MAX;
      for (int i = 0; i < l; ++i) {
         if (key->addrEqual(leaf_set[i])) continue;
         int dis = key->distance(leaf_set[i]);
         if (dis < min_dis) {
            closest = leaf_set[i];
            min_dis = dis;
         }
      }
      if (closest->addrEqual(&id)) { 
         return 0;
      } else {
         return (closest);
      }
   }
   LOG(INFO) << "the key is not in the leaf_set. key : [" << key->ip << ":" << key->port << ":" << key->id << "]";
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif

   return NULL;
}

ID *Node::lookupRouteTable(ID *key) {
   int p = id.sha_pref(key);
   int v = key->position(p);

   if (rtable[p][v]->notNull()) {
      //return rtable[p][v];
      //forward(rtable[p][v], key);
      if (!rtable[p][v]->addrEqual(&id))
         return rtable[p][v];
   } 
   int min_dis = INT_MAX;
   int pos = -1;
   for (int i = 0; i < BIT_NUM; ++i) {
      if (rtable[p][i]->notNull() && !rtable[p][i]->addrEqual(&id)) {
         int dis = rtable[p][i]->distance(key);
         if (dis < min_dis) {
            pos = i;
            min_dis = dis;
         }
      }
   }
   if (pos != -1) {
      //forward(rtable[p][pos], key);
      return rtable[p][pos];
   }
   LOG(INFO) << "error occurs with leaf set and route table";
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif
   return NULL;
}

ID *Node::lookupKey(ID *key) {
   /* check whether the key is in the leaf set */
   ID *node = lookupLeafSet(key);
   if (node) {
      return node;
   } else {
      return lookupRouteTable(key);
   }
}

int Node::lookup(ID *key, bool server) { 
   unuse(&server);
   int fd = -1;
   /* check whether the key is controled by myself. */
   if (!server) {
      map<ID *, string >::iterator it = keys.find(key);
      if (it != keys.end()) {
         if (key->ip.compare(local_ip) && key->port == local_port) {
            fd = 0;
            // the key is localed.
         } else {
            fd = links->find(key->ip, key->port);
            if (fd == -1) {
               LOG(INFO) << "No reason goes here, key[ip:" << key->ip << ",port:" << key->port << ",id:" << key->id << "].";
#ifdef DEBUG
               google::FlushLogFiles(google::INFO);
#endif
            }
         }
         return fd;
      }
   }

   /* check whether the key is in the leaf set */
   for (int i = 0; i < l; ++i) {
      if (leaf_set[i]->notNull()) {
         s = i;
         break;
      }
   }
   for (int i = l - 1; i >= 0; --i) {
      if (leaf_set[i]->notNull()) {
         f = i;
         break;
      }
   }
   if (key->bigger(leaf_set[s]) && !key->bigger(leaf_set[f])) {
      ID *closest = NULL;
      int min_dis = INT_MAX;
      for (int i = 0; i < l; ++i) {
         int dis = key->distance(leaf_set[i]);
         if (dis < min_dis) {
            closest = leaf_set[i];
            min_dis = dis;
         }
      }
      if (closest->addrEqual(&id)) { 
         return 0;
      }
      forward(closest, key);
      return -1;
   }

   /* find the closest node from route table. */
   route(key);
   return -1;
}

ID *Node::route(ID *key) {
   int p = id.sha_pref(key);
   int v = key->position(p);

   if (rtable[p][v]->notNull()) {
      forward(rtable[p][v], key); 
   } else {
      int min_dis = INT_MAX;
      int pos = -1;
      for (int i = 0; i < BIT_NUM; ++i) {
         if (rtable[p][i]->notNull() && !rtable[p][i]->addrEqual(&id)) {
            int dis = rtable[p][i]->distance(key);
            if (dis < min_dis) {
               pos = i;
               min_dis = dis;
            }
         }
      }
      if (pos != -1) forward(rtable[p][pos], key);
      else {
         LOG(INFO) << "error occurs with leaf set and route table";
#ifdef DEBUG
         google::FlushLogFiles(google::INFO);
#endif
      }
   }
   return NULL;
}

void Node::forward(ID *_id, ID *key) {
   int fd = links->find(_id->ip, _id->port);
   send(fd, FORWARD, key, NULL, 0);
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
   LOG(INFO) << "nwrite = [" << nwrite << ":" << _msg << ", len=[" << len << "],op=[" << opToStr(op) << "].";
#ifdef DEBUG
   google::FlushLogFiles(google::INFO);
#endif
}

void Node::encode(char op, ID *src, char *extra_msg, int msg_len) {
   unuse(&op);
   unuse(src);
   unuse(extra_msg);
   unuse(&msg_len);
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

int Node::serialize(ID *t_id, char *msg) {
   int offset = 0;
   int ip_len = t_id->ip.length();
   memcpy(msg + offset, &ip_len, 4);
   offset += 4;
   memcpy(msg + offset, t_id->ip.c_str(), ip_len);
   offset += ip_len;
   memcpy(msg + offset, &t_id->port, 4);
   offset += 4;
   memcpy(msg + offset, &t_id->id, 4);
   offset += 4;
   return offset;
}

int Node::deserialize(char *src, ID *t_id) {
   int offset = 0;
   char ip[32] = {0};
   int ip_len = 0;
   memcpy(&ip_len, src + offset, 4);
   offset += 4;
   memcpy(ip, src + offset, ip_len);
   offset += ip_len;
   t_id->ip = ip;
   memcpy(&t_id->port, src + offset, 4);
   offset += 4;
   memcpy(&t_id->id, src + offset, 4);
   offset += 4;
   return offset;
}

void unuse(void *) {}

void Node::daemonize() {
   int fd;

   if (fork() != 0) exit(0);
   setsid();

   if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
      dup2(fd, STDIN_FILENO);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO); 
      if (fd > STDERR_FILENO) close(fd);
   }
}

string Node::opToStr(char op) {
   switch (op) {
   case START_PULL:
      return "START_PULL";
   case FIRST_PUSH:
      return "FIRST_PUSH";
   case FORWARD:
      return "FORWARD";
   case FW_RESP:
      return "FW_RESP";
   case PULL:
      return "PULL";
   case SYNC:
      return "SYNC";
   case SYNC_RT:
      return "SYNC_RT";
   case SYNC_RT_BACK:
      return "SYNC_RT_BACK";
   default:
      return "Unknown";
   }
}
