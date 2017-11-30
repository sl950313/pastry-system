/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : set.h
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/

#ifndef SET_H
#define SET_H
#include "id.h"

class Set {
    public:
        Set();
        ~Set();

        int serialize(ID *t_id, char *msg);
        int deserialize(char *str, ID *t_id);
};

#endif /* SET_H */


