#ifndef NODE_H
#define NODE_H
#include <string>
#include <vector>
#include <map>
#include <list>
#include "link.h"
#include "sha1.hpp"
#include "id.h"

using namespace std;

void unuse(void *);// {};

enum Role {Client, Server};
//typedef char ID[4];
//typedef int ID;


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
   void deleteNode(Each_link *el);

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
   map<ID *, string, idCmp> keys;

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
   void broadcast(char op, void *message, int msg_len);

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

   char *keyIsLocaled(ID *key);
   void printLeafSetAndRouteTable(int fd);
   void printKeysMap();
};

class Server : public Node {
};

class Client : public Node {
};

#endif 
