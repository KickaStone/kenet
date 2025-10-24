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

// test update channel
TEST_F(PollerTest, update_channel_test) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    
    Channel channel(&poller, pipefd[0]);
    
    channel.setEvents(EPOLLIN);
    poller.updateChannel(&channel);
    
    EXPECT_TRUE(channel.added());
    
    write(pipefd[1], "test", 4);
    
    std::vector<Channel*> activeChannels;
    int nfds = poller.poll(1000, activeChannels);
    
    EXPECT_EQ(nfds, 1);
    EXPECT_EQ(activeChannels.size(), 1);
    EXPECT_EQ(activeChannels[0]->fd(), pipefd[0]);
    EXPECT_EQ(activeChannels[0]->revents(), EPOLLIN);
    
    poller.removeChannel(&channel);
    EXPECT_FALSE(channel.added());
    
    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(PollerTest, epollout_test) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    
    Channel channel(&poller, pipefd[1]);
    
    channel.setEvents(EPOLLOUT);
    poller.updateChannel(&channel);
    
    std::vector<Channel*> activeChannels;
    int nfds = poller.poll(1000, activeChannels);
    
    EXPECT_EQ(nfds, 1);
    EXPECT_EQ(activeChannels.size(), 1);
    EXPECT_EQ(activeChannels[0]->fd(), pipefd[1]);
    EXPECT_EQ(activeChannels[0]->revents(), EPOLLOUT);
    
    poller.removeChannel(&channel);
    close(pipefd[0]);
    close(pipefd[1]);
}

TEST_F(PollerTest, channel_state_test) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    
    Channel channel(&poller, pipefd[0]);
    
    EXPECT_FALSE(channel.added());
    EXPECT_EQ(channel.events(), 0);
    
    channel.setEvents(EPOLLIN);
    poller.updateChannel(&channel);
    
    EXPECT_TRUE(channel.added());
    EXPECT_EQ(channel.events(), EPOLLIN);
    
    poller.removeChannel(&channel);
    EXPECT_FALSE(channel.added());
    EXPECT_EQ(channel.events(), EPOLLIN);
    
    close(pipefd[0]);
    close(pipefd[1]);
}