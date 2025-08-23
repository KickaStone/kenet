//
// Created by jijuncheng on 8/23/25.
//

#include <thread>
#include <gtest/gtest.h>
#include "../../include/logger/ring_buffer.h"

TEST(ringBufferTest_simple, TEST_1) {
    SpscRing<size_t, 1024> buffer;

    for (size_t i = 0; i < 1024; i++) {
        buffer.try_push(i);
    }

    for (size_t i = 0; i < 1024; i++) {
        size_t value;
        buffer.try_pop(value);
        ASSERT_EQ(value, i);
    }
}

TEST(ringBufferTest_simple, TEST_2) {
    SpscRing<size_t, 1024> buffer;

    auto func1 = [&buffer]() {
        for (size_t i = 0; i < 1024; i++) {
            buffer.try_push(i);
        }
    };

    auto func2 = [&buffer]() {
        for (size_t i = 0; i < 1024; i++) {
            size_t value;
            buffer.try_pop(value);
            ASSERT_EQ(value, i);
        }
    };

    std::thread t1(func1);
    std::thread t2(func2);

    t1.join();
    t2.join();
}
