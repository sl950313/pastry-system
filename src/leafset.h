/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : leafset.h
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/

#ifndef LEAF_SET_H
#define LEAF_SET_H
#include "id.h"
#include "link.h"
#include "set.h"

struct Each_link;

class LeafSet : public Set{
    public:
        LeafSet(int _l, ID *_id);
        ~LeafSet();
        ID *lookupLeafSet(ID *key);
        void updateLeafSetWithNewNode(Each_link *el);
        void updateLeafSetWithDeleteNode(Each_link *el);
        void correctLeafSet();
        int serializeLeafSet(char *str);
        int deserializeLeafSet(char **str);
        void printLeafSet();

    private:
        LeafSet();
        void initial();


        ID **leaf_set;
        ID *id;
        int s, f, l;
        int right, left;
};

#endif /* LEAF_SET_H */
