# **pastry-system**
__Pastry__ p2p architecture which aims to be used in parameter server [ps-lite](https://github.com/dmlc/ps-lite).

An implement of [Pastry: Scalable, decentralized object location, and routing for large-scale peer-to-peer systems](https://link.springer.com/chapter/10.1007/3-540-45518-3_18)

```
void push(char *msg);
```
The only interface will push the msg to the total node in the pastry system.

# Build

```
  git clone https://github.com/sl950313/pastry-system
  cd pastry-system 
  ./autogen.sh && ./configure && make -j4 
  make check (Optimal)
  make install
``` 

# Dependencies

- [sha1](https://github.com/vog/sha1)
- [google glog](https://github.com/google/glog)
- [protobuf](https://github.com/google/protobuf)
- [google test](https://github.com/google/googletest)

# How To Use 

A main can be written like this:
```
void main() {
   /*
   One node should be known to myself so that I can join the system.
   */
   Node *p = new Node(role, ip, port);
   p->init();
   p->joinPastry(other_nodeip, port);
   /*
   The boot() will create a therad and start listen and process network info in status machine.
   */
   p->boot();

   /*
   You can only call push to push the messages to other node in the system through DHT pastry.
   */
   p->push("Hello World!");
}
```



``` 
./pastry role port [serv_ip] [serv_port] 
./pastry 1 8001 
./pastry 0 8002 127.0.0.1 8001 
```

# License
100 % Public Domain

# Authors
- Lei Shi
