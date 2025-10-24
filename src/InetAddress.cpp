#include "InetAddress.h"

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); // convert port to network byte order
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // convert ip to network byte order
}

/**
 * When not given an ip, use INADDR_ANY to listen on all interfaces.
 */
InetAddress::InetAddress(uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = INADDR_ANY;
}

/**
 * Construct with a sockaddr_in.
 */
InetAddress::InetAddress(const sockaddr_in& addr) {
    addr_ = addr;
}

/**
 * Convert the address to a string.
 * @return The address as a string.
 */
std::string InetAddress::toIp() const {
    return inet_ntoa(addr_.sin_addr);
}

/**
 * Convert the port to a uint16_t.
 * @return The port as a uint16_t.
 */
uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

/**
 * Convert the address and port to a string.
 * @return The address and port as a string.
 */
std::string InetAddress::toIpPort() const {
    return toIp() + ":" + std::to_string(toPort());
}


/**
 * @return The socket address.
 */
const sockaddr_in* InetAddress::getSockAddr() const {
    return &addr_;
}
