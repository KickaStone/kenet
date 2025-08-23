#include <gtest/gtest.h>
#include <vector>
#include "Poller.h" 
#include "Channel.h"


class PollerTest : public testing::Test {
    protected:
    Poller poller;
};

TEST_F(PollerTest, basic_test) {
    std::vector<Channel*> activeChannels;
    EXPECT_EQ(poller.poll(1000, activeChannels), 0);
    EXPECT_EQ(activeChannels.size(), 0);
}


// test with a channel
TEST_F(PollerTest, channel_test) {
    Channel channel(&poller, 0);
    channel.setReadCallback([]() {
        GTEST_LOG_(INFO) << "Channel read callback";
    });
    poller.updateChannel(&channel);
    std::vector<Channel*> activeChannels;
    EXPECT_EQ(poller.poll(1000, activeChannels), 0);
}