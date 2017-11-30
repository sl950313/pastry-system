/***********************************************
# File Name: defines.h
# 
# Author: Lei Shi
# Mail: sl950313@gmail.com
# Create: 2017-11-23 13:04:38
# Last Modified: 2017-11-23 13:04:38
***********************************************/

#ifndef _DEFINES_H
#define _DEFINES_H

#define LOG_FILENAME "pastry-"
#define BIT_NUM 4

/* For debug mode. */
#define DEBUG

enum Ops {
   START_PULL,
   FIRST_PUSH,
   FP_BACK,
   FORWARD,
   FW_RESP,
   PULL,
   SYNC,
   SYNC_RT,
   SYNC_RT_BACK,
   NEW_NODE
};
#endif 
