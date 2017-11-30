/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : leafset.cpp
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/

#include <limits.h>
#include <glog/logging.h>
#include "leafset.h"

LeafSet::LeafSet() {
    leaf_set = (ID **)malloc(sizeof(ID *) * l);
    for (int i = 0; i < l; ++i)
        leaf_set[i] = new ID();
    leaf_set[l / 2]->copy(id);
}

LeafSet::~LeafSet() { 
    for (int i = 0; i < l; ++i)
        delete leaf_set[i];
    delete leaf_set;
}

ID *LeafSet::lookupLeafSet(ID *key) {
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
        for (int i = s; i <= f; ++i) {
            if (key->addrEqual(leaf_set[i])) continue;
            int dis = key->distance(leaf_set[i]);
            if (dis < min_dis) {
                closest = leaf_set[i];
                min_dis = dis;
            }
        }
        if (closest->addrEqual(id)) { 
            return closest;
        } else {
            return (closest);
        }
    }
    LOG(INFO) << "the key is not in the leaf_set. key : [" << key->ip << ":" << key->port << ":" << key->id << "]";
    printf("the key is not in the leaf_set. key : [%s:%d:%d]\n", key->ip.c_str(), key->port, key->id);
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif

    return NULL;
}

void LeafSet::updateLeafSetWithNewNode(Each_link *el) {
    ID *t_id = new ID();
    t_id->ip = el->ip;
    t_id->port = el->port;
    ID::defaultMakeID(t_id);
    if (t_id->bigger(id)) {
        for (int i = l / 2 + 1; i < l; ++i) {
            if (!leaf_set[i]->notNull()) {
                leaf_set[i]->copy(t_id);
                f++;
            } else if (leaf_set[i]->bigger(t_id)) { 
                for (int j = i + 1; j < l; ++j) {
                    leaf_set[j]->copy(leaf_set[j - 1]);
                }
                leaf_set[i]->copy(t_id);
                f++;
            }
        }
    } else {
        for (int i = l / 2 - 1; i >= 0; --i) {
            if (!leaf_set[i]->notNull()) {
                leaf_set[i]->copy(t_id);
                s--;
            } else if (!leaf_set[i]->bigger(t_id)) { 
                for (int j = i - 1; j >= 0; --j) {
                    leaf_set[j]->copy(leaf_set[j + 1]);
                }
                leaf_set[i]->copy(t_id);
                s--;
            }
        }
    }
    delete t_id;
}

void LeafSet::correctLeafSet() {
    int index = -1;
    for (int i = 0; i < l; ++i) {
        if (leaf_set[i]->id >= id->id) {
            index = i;
            break;
        }
    }
    if (index == -1) index = l;
    ID *tmpId = new ID();
    if (index <= l / 2) {
        for (int i = 0; i < l / 2 - 1 - index; ++i)
            leaf_set[i]->copy(tmpId);
        for (int i = index - 1; i >= 0; --i) {
            leaf_set[l / 2 - 1 - (index - 1 - i)]->copy(leaf_set[i]);
        }
        for (int i = index + l / 2 - 1; i >= index; --i) {
            leaf_set[i - (index + l / 2 - 1) + l - 1]->copy(leaf_set[i]);
        }
    } else {
        for (int i = index - 1 - (l / 2 - 1); i <= index - 1; ++i) {
            leaf_set[i - (index - l / 2)]->copy(leaf_set[i]);
        }
        for (int i = index; i <= l - 1; ++i) {
            leaf_set[i - index + l / 2]->copy(leaf_set[i]);
        }
        for (int i = 3 * l / 2 - index; i < l; ++i)
            leaf_set[i]->copy(tmpId);
    }
    delete tmpId;
    if (!leaf_set[l / 2]->addrEqual(id)) {
        for (int i = l / 2; i < l - 1; ++i)
            leaf_set[i + 1]->copy(leaf_set[i]);
        leaf_set[l / 2]->copy(id);
    }
}

int LeafSet::serializeLeafSet(char *str) {
    int offset = 0;
    for (int i = 0; i < l; ++i) {
        offset += serialize(leaf_set[i], str + offset);
    }

    for (int i = 0; i < offset; ++i) {
        printf("%x", str[i]);
    }
    printf("\n");
    return offset;
}

int LeafSet::deserializeLeafSet(char **str) {
    int offset = 0;
    for (int i = 0; i < l; ++i) {
        offset += deserialize(*str + offset, leaf_set[i]);
    }
    *str += offset;
    return offset;
}

void LeafSet::printLeafSet() {
    printf("----------Leaf Set Info---------\n");
    for (int i = 0; i < l; ++i) {
        printf("[ip:%s   port:%d   id:%d]\n", leaf_set[i]->ip.c_str(), leaf_set[i]->port, leaf_set[i]->id);
    }
}
