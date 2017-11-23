# pastry-system
Pastry p2p architecture which aims to be used in parameter server ps-lite.

```
void push(char *msg);
```
The only interface will push the msg to the total node in the pastry system.

# Build
   
> git clone https://github.com/sl950313/pastry-system

> cd pastry-system && make -j4

# Dependencies

- [sha1](https://github.com/vog/sha1)
- [google glog](https://github.com/google/glog)

# How To Use 

A main can be written like this:
```
void main() {
   /*
   One node should be known to myself so that I can join the system.
   */
   Node *p = new Node(role, ip, port);
   p->init();
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

> make 

> ./pastry role port [serv\_ip] [serv_port] 

> ./pastry 1 8001 

> ./pastry 0 8002 127.0.0.1 8001


