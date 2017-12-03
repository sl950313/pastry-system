/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : route-table.cpp
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/

#include "route-table.h"
#include <stdlib.h>
#include <limits.h>
#include <glog/logging.h>

RouteTable::RouteTable() {
}

RouteTable::~RouteTable() {
    for (int i = 0; i < BIT_NUM; ++i) {
        for (int j = 0; j < b; ++j)
            delete rtable[i][j];
        free(rtable[i]);
    }
    free(rtable);
}

void RouteTable::initial() {
    rtable = (ID ***)malloc(sizeof(ID **) * BIT_NUM);
    for (int i = 0; i < BIT_NUM; ++i) {
        //rtable[i] = new ID();
        rtable[i] = (ID **)malloc(sizeof(ID) * b);
        for (int j = 0; j < b; ++j) {
            rtable[i][j] = new ID();
        }
        rtable[i][id->position(i)]->copy(id);// = id;
    }
}

ID *RouteTable::lookupRouteTable(ID *key) {
    int p = id->sha_pref(key);
    int v = key->position(p);

    if (p == b) {
        return id;
    }

    if (rtable[p][v]->notNull()) {
        if (!rtable[p][v]->addrEqual(id))
            return rtable[p][v];
    } 
    int min_dis = id->numericalDistance(key);
    //int pos = -1;
    ID *closest = id;
    int best_dis = id->distance(key);
    for (int i = 0; i < BIT_NUM; ++i) {
        if (rtable[p][i]->notNull() && !rtable[p][i]->addrEqual(id)) {
            int numdis = rtable[p][i]->numericalDistance(key);
            int dis = rtable[p][i]->distance(key);
            if (dis < best_dis || (dis == best_dis && min_dis > numdis)) {
                //pos = i;
                closest = rtable[p][i];
                min_dis = numdis;
                best_dis = dis;
            }
        }
    }
    if (closest) {
        return closest;
    }
    LOG(INFO) << "error occurs with leaf set and route table";
    printf("We may assume the current system doesn't has other node, error occurs with leaf set and route table\n");
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    return id;
}

void RouteTable::updateRouteTableWithNewNode(Each_link *el) {
    ID *t_id = new ID();
    t_id->ip = el->ip;
    t_id->port = el->port;
    ID::defaultMakeID(t_id); 
    int p = id->sha_pref(t_id);
    int v = t_id->position(p);

    if (!rtable[p][v]->notNull()) {
        rtable[p][v]->copy(t_id);
    }
    delete t_id;
}

void RouteTable::updateRouteTableWithDeleteNode(Each_link *el) { 
    ID *t_id = new ID();
    t_id->ip = el->ip;
    t_id->port = el->port;
    ID::defaultMakeID(t_id); 
    int p = id->sha_pref(t_id);
    int v = t_id->position(p);

    ID *tmp = new ID();
    if (rtable[p][v]->notNull()) {
        rtable[p][v]->copy(tmp);
    }

    delete tmp;
    delete t_id;
}

void RouteTable::correctRouteTable() {
    for (int i = 0; i < BIT_NUM; ++i) {
        int index = id->position(i);
        rtable[i][index]->copy(id);
    }
}

int RouteTable::serializeRouteTable(char *str) {
    int offset = 0;
    for (int i = 0; i < BIT_NUM; ++i) {
        for (int j = 0; j < b; ++j) {
            offset += serialize(rtable[i][j], str + offset);
        }
    }
    for (int i = 0; i < offset; ++i) {
        printf("%x,", str[i]);
    } 
    return offset;
}

int RouteTable::deserializeRouteTable(char **str) {
    int offset = 0;
    for (int i = 0; i < BIT_NUM; ++i)
        for (int j = 0; j < b; ++j)
            offset += deserialize(*str + offset, rtable[i][j]); 
    return offset;
}

void RouteTable::printRouteTable() {
    printf("------------Route Table Info-------------\n");
    for (int i = 0; i < b; ++i) {
        for (int j = 0; j < BIT_NUM; ++j) {
            printf("[ip:%s   port:%d   id:%d]\t", rtable[i][j]->ip.c_str(), rtable[i][j]->port, rtable[i][j]->id);
        }
        printf("\n");
    }
}
