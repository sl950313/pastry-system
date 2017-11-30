/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : route-table.h
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/

#ifndef ROUTE_TABLE_H
#define ROUTE_TABLE_H
#include "set.h"
#include "link.h"
#include "defines.h"

struct Each_link;

class RouteTable : public Set {
    public:
        RouteTable();
        RouteTable(int _b, ID *_id) {b = _b; id = _id; RouteTable();}
        ~RouteTable();

        ID *lookupRouteTable(ID *key);
        void updateRouteTableWithNewNode(Each_link *el);
        void correctRouteTable();
        int serializeRouteTable(char *str);
        int deserializeRouteTable(char **str);
        void printRouteTable();

    private:
        ID ***rtable, *id;
        int b;
};

#endif 
