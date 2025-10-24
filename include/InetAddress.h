/**
 * @file InetAddress.h
 * @brief InetAddress class keeps the address and port for a socket for easy use.
 * @date 2025-10-25
 */

#ifndef _INETADDRESS_H_
#define _INETADDRESS_H_

#include <string>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

/**
 * InetAddress class keeps the address and port for a socket for easy use.
 */
class InetAddress {
public:
    InetAddress() = default;
    InetAddress(const std::string& ip, uint16_t port);
    InetAddress(uint16_t port);
    InetAddress(const sockaddr_in& addr);
    
    /**
     * Convert the address and port to a string.
     */
    std::string toIp() const;

    /**
     * Convert the port to a uint16_t.
     */
    uint16_t toPort() const;

    /**
     * Convert the address and port to a string.
     */
    std::string toIpPort() const;

    /**
     * Return the pointer to the private sockaddr_in struct.
     */
    const sockaddr_in* getSockAddr() const;
private:
    sockaddr_in addr_;
};


#endif