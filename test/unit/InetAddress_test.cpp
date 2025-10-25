#include <gtest/gtest.h>
#include "InetAddress.h"

TEST(InetAddressTest, ip_port_test) {
    InetAddress addr("127.0.0.1", 8080);
    EXPECT_EQ(addr.toIp(), "127.0.0.1");
    EXPECT_EQ(addr.toPort(), 8080);
}

TEST(InetAddressTest, port_test) {
    InetAddress addr(8080);
    EXPECT_EQ(addr.toIp(), "0.0.0.0");
    EXPECT_EQ(addr.toPort(), 8080);
}