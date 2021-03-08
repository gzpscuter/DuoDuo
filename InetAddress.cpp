//
// Created by gzp on 2021/3/1.
//

#include <iostream>
#include "InetAddress.h"

namespace duoduo {

//    static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
//                  "InetAddress is same size as sockaddr_in6");
    static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
    static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
    static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
    static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

    // INADDR_ANY use (type)value casting.
    static const in_addr_t kInaddrAny = INADDR_ANY;
    static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

    InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
            : isIpV6_(ipv6)
    {
        static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
        static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof(addr6_));
            addr6_.sin6_family = AF_INET6;
            in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
            addr6_.sin6_addr = ip;
            addr6_.sin6_port = htons(port);
        }
        else
        {
            memset(&addr_, 0, sizeof(addr_));
            addr_.sin_family = AF_INET;
            in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
            addr_.sin_addr.s_addr = htonl(ip);
            addr_.sin_port = htons(port);
        }
    }

    InetAddress::InetAddress(const std::string &ip, uint16_t port, bool ipv6)
            : isIpV6_(ipv6)
    {
        static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
        static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
        if (ipv6)
        {
            memset(&addr6_, 0, sizeof(addr6_));
            addr6_.sin6_family = AF_INET6;
            addr6_.sin6_port = htons(port);
            if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0)
            {
                // LOG_SYSERR << "sockets::fromIpPort";
                // abort();
                std::cout << "sockets::ipv6Netendian transfer failed"<<std::endl;
                exit(1);
            }
        }

        else
        {
            memset(&addr_, 0, sizeof(addr_));
            addr_.sin_family = AF_INET;
            addr_.sin_port = htons(port);
            if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
            {
                std::cout << "sockets::ipv4Netendian transfer failed"<<std::endl;
                exit(1);
            }
        }
    }


    std::string InetAddress::toIpPort() const
    {
        char buf[64] = "";
        uint16_t port = toPort();
        snprintf(buf, sizeof(buf), ":%u", port);
        return toIp() + static_cast<std::string>(buf);
    }

    std::string InetAddress::toIp() const
    {
        char buf[64];
        if (addr_.sin_family == AF_INET)
        {
            ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
        }
        else if (addr_.sin_family == AF_INET6)
        {
            ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof(buf));
        }

        return buf;
    }

    uint16_t InetAddress::toPort() const
    {
        return ntohs(portNetEndian());
    }

    uint32_t InetAddress::ipNetEndian() const
    {
        return addr_.sin_addr.s_addr;
    }

    const uint32_t *InetAddress::ip6NetEndian() const
    {
        return addr6_.sin6_addr.s6_addr32;
    }

    static __thread char t_resolveBuffer[64 * 1024];

    bool InetAddress::resolve(std::string& hostname, InetAddress* out)
    {
        assert(out != NULL);
        struct hostent hent;
        struct hostent* he = NULL;
        int herrno = 0;
        memset(&hent, 0, sizeof(hent));

        int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
        if (ret == 0 && he != NULL)
        {
            assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
            out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
            return true;
        }
        else
        {
            if (ret)
            {
                perror("hostname resolved failed");
                exit(1);
            }
            return false;
        }
    }
}