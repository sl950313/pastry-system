/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : id.h
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-27
*
================================================================*/
#ifndef ID_H
#define ID_H

#include <string>
#include <stdio.h>
#include "sha1.hpp"

using namespace std;

class ID {
public:
   ID() {
      id = -1;
      b = 4;
      port = -1;
   }
   ID(string _ip, int _p) {
       ip = _ip;
       port = _p;
       b = 4;
   }
   ID(int _id) {
       id = _id;
       b = 4;
       port = -1;
   }
   int id; /* 0 < id < 256 = 8 * 16 */

   void copy(ID *other) {
      ip = other->ip;
      id = other->id;
      port = other->port;
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

   bool less(ID *other) {
       return id < other->id;
   }

   int distance(ID *other) {
      // for the 1.0 version, we use the sha_pref.
      return b - sha_pref(other);
   }

   int numericalDistance(ID *other) {
       return abs(id - other->id);
   }

   int position(int p) {
      return (id / _pow(b, p)) % b;
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

   int _pow(int n, int c) {
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

struct idCmp {
    bool operator()(const ID *id1, const ID *id2) const {
        return id1->id > id2->id;
    }
};

#endif 
