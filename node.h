#ifndef NODE_H
#define NODE_H
#include <string>
#include <vector>
#include <map>
#include "link.h"
#include "sha1.hpp"

using namespace std;

void unuse(void *) {};

enum Role {Client, Server};
//typedef char ID[4];
//typedef int ID;
struct ID {
   int id; /* 0 < id < 256 = 8 * 16 */
   
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
      unuse(other);
      return -1;
   }

   int position(int p) {
      unuse(&p);
      return -1;
   }

   bool notNull(int p, int v) {
      unuse(&p);
      unuse(&v);
      return true;
   }

   static void makeID(string &str, ID *target) {
      SHA1 checksum;
      checksum.update(str);
      string hash = checksum.final();
      string t = hash.substr(hash.length() - 2, 2);
      target->id = t[0] * 16 + t[1];
   }
};

class Link;

class Node {
public:
   Node(int _role, string ip, int port, string, int);//{role = (Role)_role; b = 2; l = 8;}
   virtual ~Node();
   Role role; // 0 for Server, 1 for Client.
   Link *links;
   
   //virtual int enterRing();

   void route(void *msg, int key);
   void deliver(void *msg, int key);
   void init();
   void boot();

   void pull(ID *key);
   void push(ID *key, void *msg);
   void push();

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
   vector<ID> leaf_set;

   /* routing table */
   ID **rtable;

   /* neighborhood set */
   vector<int> nset;

   /* msgs controled by myself */
   map<ID *, string> keys;

   //void create
   ID getNodeByKey(ID *key);
   int getFdByNodeId(ID *id);
   int lookup(ID *key, bool serv = false);
   void forward(ID *id, ID *key);
   void route(ID *key);
   int sha_pref(ID *key);
   void saveMsg(char *msg, int len);
   void makeMsg(ID *id, ID *key, char op);

   void decode(char *data, char op, ID *target);
};

class Server : public Node {
};

class Client : public Node {
};

#endif 
