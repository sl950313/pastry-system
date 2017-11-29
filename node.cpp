#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <sys/types.h>
#include "node.h"
#include "sha1.hpp"
#include "defines.h"

#define BIT_NUM 4

Node::Node(int _role, string ip, int port, string servip, int servp) {
    role = (Role)_role;
    b = 4;
    l = 4;
    right = 1;
    left = 0;
    s = l / 2;
    f = l / 2;
    local_ip = ip;
    local_port = port;
    serv_ip = servip;
    serv_port = servp;
    id.ip = ip;
    id.port = port;

    /* for server */
    status = 0; // PUSH_LOOKUP_NODE.
}

Node::~Node() {
    delete links;
    for (int i = 0; i < l; ++i)
        delete leaf_set[i];
    delete leaf_set;
    for (int i = 0; i < BIT_NUM; ++i) {
        for (int j = 0; j < b; ++j)
            delete rtable[i][j];
        free(rtable[i]);
    }
    free(rtable);
}

void Node::init() {
    //daemonize();

    google::InitGoogleLogging("");
    google::SetLogDestination(google::INFO, LOG_FILENAME);

    LOG(INFO) << "Google Log System already initial"; 
    printf("Google Log System already initial\n");
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif

    char tmp[256] = {0};
    sprintf(tmp, "%s:%d", local_ip.c_str(), local_port);
    string t = tmp;
    ID::makeID(t, &id);

    links = new Link(local_ip, local_port);
    links->init();
    links->listen();

    leaf_set = (ID **)malloc(sizeof(ID *) * l);
    for (int i = 0; i < l; ++i)
        leaf_set[i] = new ID();
    leaf_set[l / 2]->copy(id);
    rtable = (ID ***)malloc(sizeof(ID **) * BIT_NUM);
    for (int i = 0; i < BIT_NUM; ++i) {
        //rtable[i] = new ID();
        rtable[i] = (ID **)malloc(sizeof(ID) * id.b);
        for (int j = 0; j < b; ++j) {
            rtable[i][j] = new ID();
        }
        rtable[i][id.position(i)]->copy(id);// = id;
    }
    nset.resize(l);

    printLeafSetAndRouteTable(0);

    if (role == Client) {
        bool res = links->connect(serv_ip, serv_port);
        //assert(res, "error connect server[%s:%d]");
        if (!res) {
            LOG(ERROR) << "error connect server[" << serv_ip << ":" << serv_port << "].";
            printf("error connect server[%s:%d]\n", serv_ip.c_str(), serv_port);
#ifdef DEBUG
            google::FlushLogFiles(google::INFO);
#endif
            exit(-1);
        }
        LOG(INFO) << "connect server[" << serv_ip << ":" << serv_port << "].";
        printf("connect server[%s:%d]\n", serv_ip.c_str(), serv_port);
#ifdef DEBUG
        google::FlushLogFiles(google::INFO);
#endif
        // SYNC op.
        ID *tmp = new ID();
        tmp->ip = serv_ip;
        tmp->port = serv_port;
        int fd = getFdByNodeId(tmp);
        write(fd, &local_port, 4);
        send(fd, SYNC, &id, NULL, 0);
        printLeafSetAndRouteTable(0);
        //status = 2;
        delete tmp;
    }

    /* boot heartbeat thread. */
}

void Node::newNode(Each_link *el) {
    updateLeafSetWithNewNode(el);
    updateRouteTableWithNewNode(el);

    char msg[64] = {0};
    sprintf(msg, "%s:%d", el->ip.c_str(), el->port);
    broadcast(NEW_NODE, msg, strlen(msg));
}

void Node::deleteNode(Each_link *el) {
}

void Node::updateLeafSetWithNewNode(Each_link *el) {
    ID *t_id = new ID();
    t_id->ip = el->ip;
    t_id->port = el->port;
    ID::defaultMakeID(t_id);
    if (t_id->bigger(&id)) {
        for (int i = l / 2 + 1; i < l; ++i) {
            if (!leaf_set[i]->notNull()) {
                leaf_set[i]->copy(*t_id);
                f++;
            } else if (leaf_set[i]->bigger(t_id)) { 
                for (int j = i + 1; j < l; ++j) {
                    leaf_set[j]->copy(*leaf_set[j - 1]);
                }
                leaf_set[i]->copy(*t_id);
                f++;
            }
        }
    } else {
        for (int i = l / 2 - 1; i >= 0; --i) {
            if (!leaf_set[i]->notNull()) {
                leaf_set[i]->copy(*t_id);
                s--;
            } else if (!leaf_set[i]->bigger(t_id)) { 
                for (int j = i - 1; j >= 0; --j) {
                    leaf_set[j]->copy(*leaf_set[j + 1]);
                }
                leaf_set[i]->copy(*t_id);
                s--;
            }
        }
    }
    delete t_id;
}

void Node::updateRouteTableWithNewNode(Each_link *el) { 
    ID *t_id = new ID();
    t_id->ip = el->ip;
    t_id->port = el->port;
    ID::defaultMakeID(t_id); 
    int p = id.sha_pref(t_id);
    int v = t_id->position(p);

    if (!rtable[p][v]->notNull()) {
        rtable[p][v]->copy(*t_id);
    }
}

void *test(void *arg) {
    Node *node = (Node *)arg;
    int c = 10;
    while (c--) {
        getchar(); 
        ID *key = (ID *)malloc(sizeof(ID));
        char msg[32] = {0};
        sprintf(msg, "Hello World %d!", rand() % 1024);
        string t = msg;
        ID::makeID(t, key);
        node->push(key, msg);
        free(key);
    }
    return NULL;
}

void Node::boot() {
    //pthread_t pid;
    //pthread_create(&pid, NULL, test, this);
    links->poll(this);
}

void Node::processNetworkMsg(char *buf, int len) { 
    LOG(INFO) << "recv buf : [" << buf << "].";
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    unuse(&len);
    char op = 0x00;// = buf[0];
    //char *data = buf + 1;
    ID *src = new ID();
    char *extra_msg = NULL;
    int msg_len = 0;
    decode(buf, op, src, &extra_msg, msg_len); 

    LOG(INFO) << "recv a " << opToStr(op) << ":[" << src->ip << ":" << src->port << "].";
    printf("recv a %s :[%s:%d], [%d:%s]\n", opToStr(op).c_str(), src->ip.c_str(), src->port, msg_len, extra_msg);
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    switch (op) {
        case START_PULL:
            {
                //ID *key = (ID *)malloc(sizeof(*key));
                ID *key = new ID();
                memcpy(&key->id, extra_msg, sizeof(int));
                key->id = atoi(extra_msg);
                printf("key->id = [%d]\n", key->id);

                printKeysMap();
                char *ret = keyIsLocaled(key);
                if (ret) { 
                    printf("The key:[%d] is already pulled. End PULL\n", key->id);
                    /*
                    links->checkConnected(src); 
                    send(src, FW_RESP, &id, ret, strlen(ret));
                    */
                } else {
                    ID *des = lookupKey(key);
                    if (des->addrEqual(&id)) {
                        // the source of key is localed.
                        // nothing to do.
                        printf("按道理这里也是不应该跑到的啊!!\n");
                    } else {
                        key->id = id.id;
                        send(des, FORWARD, src, (char *)&key->id, 4);
                        // the source is localed on node with filescription fd.
                    }
                }
                delete key;
                break;
            }
        case FIRST_PUSH:
            {
                saveMsg(extra_msg, msg_len);

                ID *key = new ID();
                string str = extra_msg;
                ID::makeID(str, key);
                //key->id = (int)extra_msg;
                ID *des = lookupKey(key);
                if (des->addrEqual(&id)) {
                    // the source of key is localed.
                    // nothing to do.
                    //links->checkConnected(src);
                    send(src, FP_BACK, &id, extra_msg, msg_len);
                } else {
                    //key->id = id.id;
                    send(des, FIRST_PUSH, src, (char *)extra_msg, msg_len);
                    // the source is localed on node with filescription fd.
                }
                delete key;
                break;
            }
        case FORWARD:
            {
                //ID *key = (ID *)malloc(sizeof(*key));
                ID *key = new ID();
                memcpy(&key->id, extra_msg, sizeof(int));

                printf("key->id = [%d]\n", key->id);
                printKeysMap();
                char *ret = keyIsLocaled(key);
                if (ret) { 
                    links->checkConnected(src);
                    send(src, FW_RESP, &id, ret, strlen(ret));
                } else {
                    ID *des = lookupKey(key);
                    if (des->addrEqual(&id)) {
                        // send(des, FW_RESP, &id, NULL, 0);
                        printf("从道理来讲不应该到这里啊.\n");
                    }
                    send(des, FORWARD, src, extra_msg, msg_len);
                }
                delete key;
                break;
            }

        case FW_RESP:
            {
                saveMsg(extra_msg, msg_len);
                /*
                   if (status == 1) { // means the RESP is received by server.
                //push(src, NULL);
                } else { // the RESP is received by client.
                // TODO: some other strategy should be added.
                if (status == 0) 
                saveMsg(extra_msg, strlen(extra_msg));
                else if (status == 2){ // means the RESP is a SYNC info.
                send(getFdByNodeId(src), SYNC_RT, &id, NULL, 0);
                }
                }
                */
                break;
            }
        case FP_BACK:
            {
                ID *key = new ID();
                string str = extra_msg;
                ID::makeID(str, key);
                printf("In FP_BACK, key->id : [%d]\n", key->id);
                char snd[4] = {0};
                sprintf(snd, "%4d", key->id);
                broadcast(START_PULL, snd, 4);
                break;
                // Here will be deleted.
            }

        case SYNC:
            {
                ID *node = lookupKey(src);
                LOG(INFO) << "after lookupKey, the ret node : [ip:" << node->ip << ",port:" << node->port << "].";
                printf("after lookupKey, the ret node : [ip:%s,port:%d]\n", node->ip.c_str(), node->port);
#ifdef DEBUG
                google::FlushLogFiles(google::INFO);
#endif
                CHECK_NOTNULL(node);
                if (node->addrEqual(&id)) {
                    char msg[1024] = {0};
                    int len = serializeLeafSetAndRouteTable(msg);
                    printf("-------After send SYNC_RT command-----\n");
                    printLeafSetAndRouteTable(1);
                    send(src, SYNC_RT, &id, msg, len);
                } else {
                    send(node, SYNC, src, NULL, 0);
                }
                break;
            }
        case SYNC_RT:
            {
                for (int i = 0; i < msg_len; ++i) {
                    printf("%x,", extra_msg[i]);
                }
                printf("\n");
                deserializeLeafSetAndRouteTable(extra_msg);
                break;
            }
        case NEW_NODE:
            {
                char *p = strchr(extra_msg, ':');
                *p = 0;
                p++;
                string t_ip = extra_msg;
                int port = atoi(p);
                if (t_ip.compare(local_ip) != 0 || port != local_port) 
                    links->connect(t_ip, port);
                break;
            }
        default:
            break;
    }
    delete src;
}

void Node::broadcast(char op, void *message, int msg_len) {
    for (int i = 0; i < 10240; ++i) {
        if (links->links[i].fd == 0 || !links->links[i].use) continue;
        send(links->links[i].fd, op, &id, (char *)message, msg_len);
    }
}

char *Node::keyIsLocaled(ID *key) {
    map<ID *, string>::iterator it = keys.find(key);
    if (it != keys.end()) return (char *)it->second.c_str();
    else return NULL;
}

void Node::deserializeLeafSetAndRouteTable(char *msg) {
    LOG(INFO) << "Updating Route Table and Leaf set.";
    printf("Updating Route Table and Leaf set.\n");
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    int offset = 0;
    for (int i = 0; i < l; ++i) {
        offset += deserialize(msg + offset, leaf_set[i]);
    }
    printf("Before Correct Leaf Set\n");
    printLeafSetAndRouteTable(0);
    correctLeafSet();
    printf("After Correct Leaf Set\n");
    printLeafSetAndRouteTable(0);

    for (int i = 0; i < BIT_NUM; ++i)
        for (int j = 0; j < b; ++j)
            offset += deserialize(msg + offset, rtable[i][j]); 
    printf("Before Correct Route Table\n");
    printLeafSetAndRouteTable(0);
    correctRouteTable(); 
    printf("After Correct Route Table\n");
    printLeafSetAndRouteTable(0);
}

int Node::serializeLeafSetAndRouteTable(char *msg) {
    int offset = 0;
    for (int i = 0; i < l; ++i) {
        offset += serialize(leaf_set[i], msg + offset);
    }

    for (int i = 0; i < BIT_NUM; ++i) {
        for (int j = 0; j < b; ++j) {
            offset += serialize(rtable[i][j], msg + offset);
        }
    }
    for (int i = 0; i < offset; ++i) {
        printf("%x,", msg[i]);
    }
    printf("\n");
    LOG(INFO) << "SYNC rtable and leaf_set total len : " << offset;
    printf("SYNC rtable and leaf_set total len : %d\n", offset);
    return offset;
}

void Node::correctRouteTable() {
    for (int i = 0; i < BIT_NUM; ++i) {
        int index = id.position(i);
        rtable[i][index]->copy(id);
    }
}

void Node::correctLeafSet() { 
    //leaf_set[l / 2]->copy(id);
    int index = -1;
    for (int i = 0; i < l; ++i) {
        if (leaf_set[i]->id >= id.id) {
            index = i;
            break;
        }
    }
    if (index == -1) index = l;
    ID *tmpId = new ID();
    if (index <= l / 2) {
        for (int i = 0; i < l / 2 - 1 - index; ++i)
            leaf_set[i]->copy(*tmpId);
        for (int i = index - 1; i >= 0; --i) {
            leaf_set[l / 2 - 1 - (index - 1 - i)]->copy(*leaf_set[i]);
        }
        for (int i = index + l / 2 - 1; i >= index; --i) {
            leaf_set[i - (index + l / 2 - 1) + l - 1]->copy(*leaf_set[i]);
        }
    } else {
        for (int i = index - 1 - (l / 2 - 1); i <= index - 1; ++i) {
            leaf_set[i - (index - l / 2)]->copy(*leaf_set[i]);
        }
        for (int i = index; i <= l - 1; ++i) {
            leaf_set[i - index + l / 2]->copy(*leaf_set[i]);
        }
        for (int i = 3 * l / 2 - index; i < l; ++i)
            leaf_set[i]->copy(*tmpId);
    }
    delete tmpId;
    if (!leaf_set[l / 2]->addrEqual(&id)) {
        for (int i = l / 2; i < l - 1; ++i)
            leaf_set[i + 1]->copy(*leaf_set[i]);
        leaf_set[l / 2]->copy(id);
    }
}

/*
 * server run the command.
 */
void Node::push(ID *key, char *message) {
    /* let the message to the right node-id*/
    switch (status) {
        case 0:
            {
                ID *node = lookupKey(key);
                saveMsg(message, strlen(message));
                if (node->addrEqual(&id)) {
                    saveMsg(message, strlen(message));
                } else {
                    send(node, FIRST_PUSH, &id, message, strlen(message));
                }
            }
            break;
        case 1:
            {
                for (int i = 0; i < 10240; ++i) {
                    if (links->links[i].fd == 0 || !links->links[i].use) continue;
                    send(links->links[i].fd, START_PULL, &id, (char *)&key->id, sizeof(key->id));
                }
                status = 0;
            }
            break;
        default:
            break;
    }
}

void Node::pull(ID *key) {
    unuse(key);
    //int fd = lookup(key);
}

ID *Node::lookupLeafSet(ID *key) {
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
        if (closest->addrEqual(&id)) { 
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

ID *Node::lookupRouteTable(ID *key) {
    int p = id.sha_pref(key);
    int v = key->position(p);

    if (rtable[p][v]->notNull()) {
        //return rtable[p][v];
        //forward(rtable[p][v], key);
        if (!rtable[p][v]->addrEqual(&id))
            return rtable[p][v];
    } 
    int min_dis = INT_MAX;
    int pos = -1;
    for (int i = 0; i < BIT_NUM; ++i) {
        if (rtable[p][i]->notNull() && !rtable[p][i]->addrEqual(&id)) {
            int dis = rtable[p][i]->distance(key);
            if (dis < min_dis) {
                pos = i;
                min_dis = dis;
            }
        }
    }
    if (pos != -1) {
        //forward(rtable[p][pos], key);
        return rtable[p][pos];
    }
    LOG(INFO) << "error occurs with leaf set and route table";
    printf("We may assume the current system doesn't has other node, error occurs with leaf set and route table\n");
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    return &id;
}

ID *Node::lookupKey(ID *key) {
    /* check whether the key is in the leaf set */
    ID *node = lookupLeafSet(key);
    if (node) {
        return node;
    } else {
        return lookupRouteTable(key);
    }
}

int Node::lookup(ID *key, bool server) { 
    unuse(&server);
    int fd = -1;
    /* check whether the key is controled by myself. */
    if (!server) {
        map<ID *, string >::iterator it = keys.find(key);
        if (it != keys.end()) {
            if (key->ip.compare(local_ip) && key->port == local_port) {
                fd = 0;
                // the key is localed.
            } else {
                fd = links->find(key->ip, key->port);
                if (fd == -1) {
                    LOG(INFO) << "No reason goes here, key[ip:" << key->ip << ",port:" << key->port << ",id:" << key->id << "].";
                    printf("No reason goes here, key[ip:%s,port:%d,id:%d]\n", key->ip.c_str(), key->port, key->id);
#ifdef DEBUG
                    google::FlushLogFiles(google::INFO);
#endif
                }
            }
            return fd;
        }
    }

    /* check whether the key is in the leaf set */
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
        for (int i = 0; i < l; ++i) {
            int dis = key->distance(leaf_set[i]);
            if (dis < min_dis) {
                closest = leaf_set[i];
                min_dis = dis;
            }
        }
        if (closest->addrEqual(&id)) { 
            return 0;
        }
        forward(closest, key);
        return -1;
    }

    /* find the closest node from route table. */
    route(key);
    return -1;
}

ID *Node::route(ID *key) {
    int p = id.sha_pref(key);
    int v = key->position(p);

    if (rtable[p][v]->notNull()) {
        forward(rtable[p][v], key); 
    } else {
        int min_dis = INT_MAX;
        int pos = -1;
        for (int i = 0; i < BIT_NUM; ++i) {
            if (rtable[p][i]->notNull() && !rtable[p][i]->addrEqual(&id)) {
                int dis = rtable[p][i]->distance(key);
                if (dis < min_dis) {
                    pos = i;
                    min_dis = dis;
                }
            }
        }
        if (pos != -1) forward(rtable[p][pos], key);
        else {
            LOG(INFO) << "error occurs with leaf set and route table";
            printf("error occurs with leaf set and route table\n");
#ifdef DEBUG
            google::FlushLogFiles(google::INFO);
#endif
        }
    }
    return NULL;
}

void Node::forward(ID *_id, ID *key) {
    int fd = links->find(_id->ip, _id->port);
    send(fd, FORWARD, key, NULL, 0);
}

void Node::nodesDiscoveryAlg() {
}

void Node::saveMsg(char *msg, int len) {
    unuse(&len);
    string _msg = msg;
    ID *key = (ID *)malloc(sizeof(*key));
    ID::makeID(_msg, key);
    keys.insert(pair<ID *,  string >(key, _msg));
    printf("current keys size:[%d]\n", (int)keys.size());
}

void Node::send(ID *des, char op, ID *src, char *message, int msg_len) {
    int fd = getFdByNodeId(des);
    send(fd, op, src, message, msg_len);
}

void Node::send(int fd, char op, ID *keyorId, char *msg, int msg_len) {
    char _msg[1024] = {0};
    int len = 1 + 1 + keyorId->ip.length() + 4 + 4 + 4 + msg_len;
    sprintf(_msg, "%4d%c%c%s%4d%4d%4d", len, op, (int)keyorId->ip.length(), keyorId->ip.c_str(), keyorId->port, keyorId->id, msg_len);
    memcpy(_msg + len - msg_len + 4, msg, msg_len);
    int nwrite = write(fd, _msg, len + 4); 
    LOG(INFO) << "nwrite = [" << nwrite << ":" << _msg << ", len=[" << len << "],op=[" << opToStr(op) << "].";
    printf("nwrite = [%d:%s], len=[%d], op=[%s], fd=[%d]\n", nwrite, _msg, len, opToStr(op).c_str(), fd);
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
}

void Node::encode(char op, ID *src, char *extra_msg, int msg_len) {
    unuse(&op);
    unuse(src);
    unuse(extra_msg);
    unuse(&msg_len);
}

void Node::decode(char *data, char &op, ID *target, char **extra_msg, int &msg_len) {
    op = data[0];
    data++;
    int ip_len = data[0];
    data++;
    char ip[32] = {0};
    memcpy(ip, data, ip_len);
    target->ip = ip;
    data += ip_len;
    char tmp[8] = {0};
    memcpy(tmp, data, 4);
    target->port = atoi(tmp);
    data += 4;
    memcpy(tmp, data, 4);
    target->id = atoi(tmp);
    data += 4;
    memcpy(tmp, data, 4);
    msg_len = atoi(tmp);
    data += 4;
    *extra_msg = data;
    /*
       switch (op) {
       case FORWARD:
       { 
       int len = 0;
       memcpy(&len, data, 4);
       char ip[32] = {0};
       memcpy(ip, data + 4, len);
       target->ip = ip;
       memcpy(&target->port, data + 4 + len, 4);
       memcpy(&target->id, data + 4 + len + 4, 4);
       break;
       }
       default:
       break;
       }
       */
}

int Node::getFdByNodeId(ID *id) {
    return links->find(id->ip, id->port);
}

/*
void Node::assert(bool assert, char *str) {
    if (!assert) {
        printf("%s\n", str);
        exit(-1);
    }
}
*/

int Node::serialize(ID *t_id, char *msg) {
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

int Node::deserialize(char *src, ID *t_id) {
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

void unuse(void *) {}

void Node::daemonize() {
    int fd;

    if (fork() != 0) exit(0);
    setsid();

    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO); 
        if (fd > STDERR_FILENO) close(fd);
    }
}

string Node::opToStr(char op) {
    switch (op) {
        case START_PULL:
            return "START_PULL";
        case FIRST_PUSH:
            return "FIRST_PUSH";
        case FORWARD:
            return "FORWARD";
        case FW_RESP:
            return "FW_RESP";
        case PULL:
            return "PULL";
        case SYNC:
            return "SYNC";
        case SYNC_RT:
            return "SYNC_RT";
        case FP_BACK:
            return "FP_BACK";
        case NEW_NODE:
            return "NEW_NODE";
        default:
            return "Unknown";
    }
}

void Node::printLeafSetAndRouteTable(int fd) {
    unuse(&fd);
    printf("----------Leaf Set Info---------\n");
    for (int i = 0; i < l; ++i) {
        printf("[ip:%s   port:%d   id:%d]\n", leaf_set[i]->ip.c_str(), leaf_set[i]->port, leaf_set[i]->id);
    }
    printf("----------Route Table Info-------\n");
    for (int i = 0; i < BIT_NUM; ++i) {
        for (int j = 0; j < b; ++j) {
            printf("[ip:%s   port:%d   id:%d]\t", rtable[i][j]->ip.c_str(), rtable[i][j]->port, rtable[i][j]->id);
        }
        printf("\n");
    }
}

void Node::printKeysMap() {
    int iter = 0;
    for (map<ID *, string>::iterator it = keys.begin(); it != keys.end(); ++it, ++iter) {
        printf("keys[%d] : [%d->%s]\n", iter, it->first->id, it->second.c_str());
    }
}
