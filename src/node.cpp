#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <sys/types.h>
#include "node.h"
#include "sha1.hpp"
#include "defines.h"

Node::Node(int _role, string ip, int port, string servip, int servp) {
    role = (Role)_role;
    b = 4;
    l = 4;
    key_file = -1;
    local_ip = ip;
    local_port = port;
    serv_ip = servip;
    serv_port = servp;
    id.ip = ip;
    id.port = port;
    ID::defaultMakeID(&id);

    lset = new LeafSet(l, &id);
    rtable = new RouteTable(b, &id);
    links = new Link(local_ip, local_port);
}

Node::~Node() {
    delete links;
    delete lset;
    delete rtable;
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

    key_file = open(TMP_FILE_NAME, O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    CHECK_NE(key_file, -1) << "file " << TMP_FILE_NAME << " create and open error";

    links->init();
    links->listen();

    lset->printLeafSet();
    rtable->printRouteTable();

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
        int n = write(fd, &local_port, 4);
        unuse(&n);
        send(fd, SYNC, &id, NULL, 0);
        delete tmp;
    }

    /* boot heartbeat thread. TODO*/
}

void Node::newNode(Each_link *el) {
    lset->updateLeafSetWithNewNode(el);
    rtable->updateRouteTableWithNewNode(el);

    char msg[64] = {0};
    sprintf(msg, "%s:%d", el->ip.c_str(), el->port);
    broadcast(NEW_NODE, msg, strlen(msg));
}

void Node::deleteNode(Each_link *el) {
    lset->updateLeafSetWithDeleteNode(el);
    rtable->updateRouteTableWithDeleteNode(el);
}

void Node::boot() {
    links->poll(this);
}

void Node::processNetworkMsg(char *buf, int len) { 
    char op = 0x00;// = buf[0];
    ID *src = new ID();
    char *extra_msg = NULL;
    int msg_len = 0;

    ID *key = new ID();
    unuse(&len);
    LOG(INFO) << "Pastry recv buf : [" << buf << "].";
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    decode(buf, op, src, &extra_msg, msg_len); 

    LOG(INFO) << "recv a " << opToStr(op) << ":[" << src->ip << ":" << src->port << "].";
    printf("recv a %s :[%s:%d], [%d:%s]\n", opToStr(op).c_str(), src->ip.c_str(), src->port, msg_len, extra_msg);
#ifdef DEBUG
    google::FlushLogFiles(google::INFO);
#endif
    switch (op) {
        case START_PULL:
            {
                memcpy(&key->id, extra_msg, sizeof(int));
                key->id = atoi(extra_msg);
                printf("key->id = [%d]\n", key->id);

#ifdef DEBUG
                printKeysMap();
#endif
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
                        //key->id = id.id;
                        send(des, FORWARD, &id, (char *)&key->id, 4);
                        // Wait for FW_RESP.
                        // the source is localed on node with filescription fd.
                    }
                }
                break;
            }
        case FIRST_PUSH:
            {
                saveMsg(extra_msg, msg_len);

                string str = extra_msg;
                ID::makeID(str, key);
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
                break;
            }
        case FORWARD:
            {
                //ID *key = (ID *)malloc(sizeof(*key));
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
                        printf("从道理来讲不应该到这里啊,因为FIRST_PUSH没有把这条消息发过来.\n");
                    } else {
                        send(des, FORWARD, src, extra_msg, msg_len);
                    }
                }
                break;
            }

        case FW_RESP:
            {
                saveMsg(extra_msg, msg_len);
                break;
            }
        case FP_BACK:
            {
                string str = extra_msg;
                ID::makeID(str, key);
                printf("In FP_BACK, key->id : [%d]\n", key->id);
                char snd[5] = {0};
                sprintf(snd, "%4d", key->id);
                broadcast(START_PULL, snd, 4);
                break;
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
                    int offset = lset->serializeLeafSet(msg);
                    offset += rtable->serializeRouteTable(msg + offset) ;//
                    LOG(INFO) << "SYNC rtable and leaf_set total len : " << offset;
                    printf("SYNC rtable and leaf_set total len : %d\n", offset);
                    printf("-------After send SYNC_RT command-----\n");
                    lset->printLeafSet();
                    rtable->printRouteTable();
                    send(src, SYNC_RT, &id, msg, offset);
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
                //deserializeLeafSetAndRouteTable(extra_msg);
                LOG(INFO) << "Updating Route Table and Leaf set.";
                printf("Updating Route Table and Leaf set.\n");
#ifdef DEBUG
                google::FlushLogFiles(google::INFO);
                printf("Before Correct Leaf Set And Route Table\n");
                lset->printLeafSet();
                rtable->printRouteTable();
#endif

                lset->deserializeLeafSet(&extra_msg);
                rtable->deserializeRouteTable(&extra_msg);
                lset->correctLeafSet();
                rtable->correctRouteTable(); 

#ifdef DEBUG
                printf("After Correct Leaf Set And Route Table\n");
                lset->printLeafSet();
                rtable->printRouteTable();
#endif
                break;
            }
        case NEW_NODE:
            {
                char *p = strchr(extra_msg, ':'); // ip:port
                *p = 0;
                p++;
                string t_ip = extra_msg;
                int port = atoi(p);
                if (t_ip.compare(local_ip) != 0 || port != local_port) {
                    printf("New node is [%s:%d]\n", t_ip.c_str(), port);
                    links->connect(t_ip, port);
                }
                break;
            }
        default:
            break;
    }
    delete key;
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


/*
 * server run the command.
 */
void Node::push(ID *key, char *message) {
    ID *node = lookupKey(key);
    saveMsg(message, strlen(message));
    if (!node->addrEqual(&id)) {
        send(node, FIRST_PUSH, &id, message, strlen(message));
    } else {
        printf("The key is in currently localed. It's time to broadcast.\n");
        char snd[5] = {0};
        sprintf(snd, "%4d", key->id);
        broadcast(START_PULL, snd, 4);
    }
}

ID *Node::lookupKey(ID *key) {
    /* check whether the key is in the leaf set */
    ID *node = lset->lookupLeafSet(key);
    if (node) {
        return node;
    } else {
        return rtable->lookupRouteTable(key);
    }
}

void Node::nodesDiscoveryAlg() {
}

void Node::saveMsg(char *msg, int len) {
    unuse(&len);
    string _msg = msg;
    ID *key = (ID *)malloc(sizeof(*key));
    ID::makeID(_msg, key);
    keys.insert(pair<ID *,  string >(key, _msg));
    printf("msg %s is current pulled, current keys size:[%d]\n", msg, (int)keys.size());
    int n = write(key_file, msg, len);
    n = write(key_file, "\n", 1);
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
}

int Node::getFdByNodeId(ID *id) {
    return links->find(id->ip, id->port);
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

void Node::printKeysMap() {
    int iter = 0;
    for (map<ID *, string>::iterator it = keys.begin(); it != keys.end(); ++it, ++iter) {
        printf("keys[%d] : [%d->%s]\n", iter, it->first->id, it->second.c_str());
    }
}
