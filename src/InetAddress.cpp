#include "../include/InetAddress.h"

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); // convert port to network byte order
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // convert ip to network byte order
}

InetAddress::InetAddress(uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = INADDR_ANY;
}

InetAddress::InetAddress(const sockaddr_in& addr) {
    addr_ = addr;
}

std::string InetAddress::toIp() const {
    return inet_ntoa(addr_.sin_addr);
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

std::string InetAddress::toIpPort() const {
    return toIp() + ":" + std::to_string(toPort());
}

const sockaddr_in* InetAddress::getSockAddr() const {
    return &addr_;
}


