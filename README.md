# pastry-system
Pastry p2p module which aims to be used in parameter server ps-lite.

# Build
   
   git clone https://github.com/sl950313/pastry-system
   cd pastry-system && make -j4

# How To Use

   ./pastry role port [serv\_ip] [serv_port]

example

   ./pastry 1 8001 
   ./pastry 0 8002 127.0.0.1 8001


