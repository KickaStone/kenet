#include <gtest/gtest.h>
#include "EventLoop.h"

class EventLoopTest : public testing::Test {
    protected:
    EventLoop loop;
    std::shared_ptr<std::atomic<int>> taskid = std::make_shared<std::atomic<int>>(0);
};

TEST_F(EventLoopTest, basic_test) {
    taskid->store(0);
    loop.runInLoop([this]() {
        taskid->fetch_add(1);
    });

    // start loop
    std::thread t([this]() {
        GTEST_LOG_(INFO) << "EventLoopTest loop start";
        loop.loop();
    });

    // wait 1 second and quit
    std::this_thread::sleep_for(std::chrono::seconds(1));
    GTEST_LOG_(INFO) << "EventLoopTest quit";
    loop.quit();

    // check taskid 
    EXPECT_EQ(taskid->load(), 1);

    t.join();
}


TEST_F(EventLoopTest, quit_test) {
    taskid->store(0);

    loop.runInLoop([this]() {
        // sleep 1 second to stuck the loop
        std::this_thread::sleep_for(std::chrono::seconds(1));
        EXPECT_EQ(taskid->load(), 0);
    });

    for (int i = 0; i < 10; i++) {
        loop.runInLoop([this]() {
            taskid->fetch_add(1);
        });
    }

    std::thread t([this]() {
        loop.loop();
    });

    // wait for loop to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    loop.quit();

    t.join();
    EXPECT_EQ(taskid->load(), 10);
}
