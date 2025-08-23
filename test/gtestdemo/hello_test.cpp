//
// Created by jijuncheng on 8/23/25.
//
#include <gtest/gtest.h>

TEST(HelloTest, BasicAssertions) {
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(7*5, 35);
}