#ifndef _NET_INETADDRESS_H_
#define _NET_INETADDRESS_H_

#include <string>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

class InetAddress {
public:
    InetAddress() = default;
    InetAddress(const std::string& ip, uint16_t port);
    InetAddress(uint16_t port);
    InetAddress(const sockaddr_in& addr);
    std::string toIp() const;
    uint16_t toPort() const;
    std::string toIpPort() const;
    const sockaddr_in* getSockAddr() const;
private:
    sockaddr_in addr_;
};


#endif