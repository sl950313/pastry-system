#ifndef NODE_H
#define NODE_H
#include <string>
#include <vector>
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include "link.h"
#include "sha1.hpp"
#include "id.h"
#include "leafset.h"
#include "route-table.h"

using namespace std;

void unuse(void *);// {};

enum Role {Client, Server};
//typedef char ID[4];
//typedef int ID;


class Link;
class LeafSet;
class RouteTable;
struct Each_link;

class Node {
public:
   Node(int _role, string ip, int port, string, int);//{role = (Role)_role; b = 2; l = 8;}
   virtual ~Node();
   Role role; // 0 for Server, 1 for Client.
   Link *links;

   void init();
   void boot();
   void newNode(Each_link *el);
   void deleteNode(Each_link *el);

   void pull(ID *key);
   void push(char *msg, int len);


   void processNetworkMsg(char *buf, int len);

private: 
   std::mutex mtx_;
   std::thread main_thread;

   string serv_ip;
   int serv_port;

   string local_ip;
   int local_port;

   /* param of pastry */
   int b;
   int l;
   ID id;

   LeafSet *lset;
   RouteTable *rtable;

   /* neighborhood set */
   vector<int> nset;

   /* msgs controled by myself */
   map<ID *, string, idCmp> keys;
   int key_file; // save keys of map in a file.

   void process(char *buf, int len);
   void push(ID *key, char *msg);
   void lock() {
       mtx_.lock();
   }
   void unlock() {
       mtx_.unlock();
   }

   /*
    * Lookup key from leaf set and route table
    * param
    *    key : the key to be lookup
    *
    * return valute:
    *    ID * : NULL if the key is localed, else the node id where the lookup command should be send to 
    */
   ID *lookupKey(ID *key);
   int getFdByNodeId(ID *id);
   void saveMsg(char *msg, int len);
   void send(int fd, char op, ID *keyorId, char *message, int msg_len);
   void send(ID *des, char op, ID *src, char *message, int msg_len);
   void broadcast(char op, void *message, int msg_len);

   void encode(char op, ID *src, char *extra_msg, int msg_len);
   void decode(char *data, char &op, ID *target, char **extra_msg, int &msg_len);

   int serialize(ID *t_id, char *target);
   int deserialize(char *src, ID *t_id);

   void daemonize();
   string opToStr(char op);

   char *keyIsLocaled(ID *key);
   void printKeysMap();
};

class Server : public Node {
};

class Client : public Node {
};

#endif 
