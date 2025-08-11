#ifndef _NET_INETADDRESS_H_
#define _NET_INETADDRESS_H_

#include <string>
#include <netinet/in.h>

class InetAddress {
public:
    InetAddress(const std::string& ip, uint16_t port);
    InetAddress(uint16_t port);
    std::string toIp() const;
    uint16_t toPort() const;
    std::string toIpPort() const;
    const sockaddr_in* getSockAddr() const;
};


#endif