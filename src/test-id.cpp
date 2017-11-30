/*================================================================
*   Copyright (C) 2017 All rights reserved.
*   
*   File Name : test-id.cpp
*   Author : Lei Shi
*   EMail : sl950313@gmail.com
*   Create : 2017-11-29
*
================================================================*/
#include <gtest/gtest.h>
#include "id.h"

class IDTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
            id = new ID(1 * 4 + 2 * 4 * 4 + 3 * 4 * 4 * 4);
            id2 = new ID(1 * 4 + 2 * 4 * 4 + 2 * 4 * 4 * 4);
        }
        virtual void TearDown() {
            delete id;
            delete id2;
        }

        ID *id, *id2;
};

TEST_F(IDTest, PositionTest) {
    for (int i = 0; i < 4; ++i) 
        EXPECT_EQ(i, id->position(i));
}

TEST_F(IDTest, ShaPrefTest) {
    EXPECT_EQ(3, id->sha_pref(id2));
}

TEST_F(IDTest, PowTest) {
    EXPECT_EQ(27, id->_pow(3, 3));
    EXPECT_EQ(9, id->_pow(3, 2));
    EXPECT_EQ(3, id->_pow(3, 1));
    EXPECT_EQ(1, id->_pow(3, 0));
}

TEST_F(IDTest, AddrEqualTest) {
    ID *i1 = new ID((string)"127.0.0.1", 8001);
    ID *i2 = new ID((string)"127.0.0.1", 8001); 
    ID *i3 = new ID((string)"127.0.0.1", 8003); 
    ID *i4 = new ID((string)"10.0.1.45", 8003); 
    ASSERT_TRUE(id->addrEqual(id2));
    ASSERT_TRUE(i1->addrEqual(i2));
    ASSERT_FALSE(i1->addrEqual(i3));
    ASSERT_FALSE(i3->addrEqual(i4));
}

TEST_F(IDTest, DistanceTest) {
    EXPECT_EQ(1, id->distance(id2));

    ID *id3 = new ID(1 + 1 * 4 + 2 * 4 * 4 + 3 * 4 * 4 * 4); // (1123) - (0123) = 4
    EXPECT_EQ(4, id3->distance(id));
    delete id3;
    ID *id4 = new ID(0 + 2 * 4 + 2 * 4 * 4 + 3 * 4 * 4 * 4); // (0223) - (0123) = 3
    EXPECT_EQ(3, id4->distance(id));
    delete id4; 
    ID *id5 = new ID(0 + 1 * 4 + 1 * 4 * 4 + 3 * 4 * 4 * 4); // (0113) - (0123) = 2
    EXPECT_EQ(2, id5->distance(id));
    delete id5;
    ID *id6 = new ID(0 + 1 * 4 + 2 * 4 * 4 + 2 * 4 * 4 * 4); // (0122) - (0123) = 1
    EXPECT_EQ(1, id6->distance(id));
    delete id6;
    EXPECT_EQ(0, id->distance(id)); // (0123) - (0123) = 0
}
