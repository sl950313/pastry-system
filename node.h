#ifndef NODE_H
#define NODE_H
#include <string>
#include <vector>
#include <map>
#include <list>
#include "link.h"
#include "sha1.hpp"

using namespace std;

void unuse(void *);// {};

enum Role {Client, Server};
//typedef char ID[4];
//typedef int ID;
struct ID {
   ID() {
      id = -1;
      b = 4;
      port = -1;
   }
   int id; /* 0 < id < 256 = 8 * 16 */

   void copy(ID &other) {
      ip = other.ip;
      id = other.id;
      port = other.port;
   }
   
   bool operator<(ID other) const {
      return id > other.id;
   }

   /* for key. */
   string ip;
   int port;

   bool bigger(ID *other) {
      return id > other->id;
   }

   int distance(ID *other) {
      // for the 1.0 version, we use the sha_pref.
      unuse(other);
      return b - sha_pref(other);
   }

   int position(int p) {
      return (id / pow(b, p)) % b;
   }

   int sha_pref(const ID *other) { 
      int res = 0;
      int src_id = id, des_id = other->id;
      for (int i = 0; i < b; ++i) {
         if ((src_id % 4) == (des_id % 4)) {
            res++;
         } else {
            break;
         }
         src_id /= 4;
         des_id /= 4;
      }
      return res;
   }

   bool notNull() {
      return id != -1;
   }

   int pow(int n, int c) {
      int sum = 1;
      while(c--) {
         sum *= n;
      }
      return sum;
   }

   bool addrEqual(ID *other) {
      return ip.compare(other->ip) == 0 && port == other->port;
   }

   static void defaultMakeID(ID *target) {
      char str[256] = {0};
      sprintf(str, "%s:%d", target->ip.c_str(), target->port);
      string s = str;
      makeID(s, target);
   }

   static void makeID(string &str, ID *target) {
      SHA1 checksum;
      checksum.update(str);
      string hash = checksum.final();
      string t = hash.substr(hash.length() - 2, 2);
      target->id = t[0] * 16 + t[1];
   }
   int b;
   //static int b;
};


class Link;
struct Each_link;

class Node {
public:
   Node(int _role, string ip, int port, string, int);//{role = (Role)_role; b = 2; l = 8;}
   virtual ~Node();
   Role role; // 0 for Server, 1 for Client.
   Link *links;
   
   //virtual int enterRing();

   ID *route(void *msg, int key);
   void deliver(void *msg, int key);
   void init();
   void boot();
   void newNode(Each_link *el);

   void pull(ID *key);
   void push(ID *key, char *msg);

   void processNetworkMsg(char *buf, int len);

private: 
   string serv_ip;
   int serv_port;

   string local_ip;
   int local_port;

   /* param of pastry */
   int b;
   int l;
   ID id;

   /* leaf set */
   ID **leaf_set;
   int s, f;
   int right;
   int left;

   /* routing table */
   ID ***rtable;

   /* neighborhood set */
   vector<int> nset;

   /* msgs controled by myself */
   map<ID *, string> keys;

   void assert(bool assert, char *str);
   ID *lookupKey(ID *key);
   void nodesDiscoveryAlg();
   //void create
   //ID getNodeByKey(ID *key);

   /*
    * Lookup key from leaf set and route table
    * param
    *    key : the key to be lookup
    *
    * return valute:
    *    ID * : NULL if the key is localed, else the node id where the lookup command should be send to 
    */
   ID *lookupLeafSet(ID *key);
   ID *lookupRouteTable(ID *key);

   int getFdByNodeId(ID *id);
   int lookup(ID *key, bool serv = false);
   void forward(ID *id, ID *key);
   ID *route(ID *key);
   //int sha_pref(ID *key);
   void saveMsg(char *msg, int len);
   //void makeMsg(ID *id, ID *key, char op);
   void send(int fd, char op, ID *keyorId, char *message, int msg_len);
   void send(ID *des, char op, ID *src, char *message, int msg_len);

   void encode(char op, ID *src, char *extra_msg, int msg_len);
   void decode(char *data, char &op, ID *target, char **extra_msg, int &msg_len);

   int serialize(ID *t_id, char *target);
   int deserialize(char *src, ID *t_id);

   void updateLeafSetWithNewNode(Each_link *el);
   void updateRouteTableWithNewNode(Each_link *el);
   void correctLeafSet();
   void correctRouteTable();
   int serializeLeafSetAndRouteTable(char *str);
   void deserializeLeafSetAndRouteTable(char *str);

   void daemonize();
   string opToStr(char op);

   int status; // 0->1 PUSH_LOOKUP_NODE -> PUSH_KEY

   void printLeafSetAndRouteTable(int fd);
};

class Server : public Node {
};

class Client : public Node {
};

#endif 
