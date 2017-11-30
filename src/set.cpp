/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : set.cpp
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/

#include "set.h"
#include <string.h>

Set::Set() {
}

Set::~Set() {
}

int Set::serialize(ID *t_id, char *msg) {
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

int Set::deserialize(char *src, ID *t_id) {
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
