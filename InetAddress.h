//
// Created by gzp on 2021/3/1.
//

#ifndef DUODUO_INETADDRESS_H
#define DUODUO_INETADDRESS_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <assert.h>
#include <netdb.h>

namespace duoduo{
    class InetAddress {
    public:
        explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

        InetAddress(const std::string& ip, uint16_t port, bool ipv6 = false);

        explicit InetAddress(const struct sockaddr_in& addr)
                : addr_(addr)
        { }

        explicit InetAddress(const struct sockaddr_in6& addr)
                : addr6_(addr), isIpV6_(true)
        { }

        inline sa_family_t family() const
        {
            return addr_.sin_family;
        }

        std::string toIp() const;

        std::string toIpPort() const;

        uint16_t toPort() const;

        inline bool isIpV6() const
        {
            return isIpV6_;
        }

        const struct sockaddr *getSockAddr() const
        {
            return static_cast<const struct sockaddr *>((void *)(&addr6_));
        }

        void setSockAddrInet6(const struct sockaddr_in6 &addr6)
        {
            addr6_ = addr6;
            isIpV6_ = (addr6_.sin6_family == AF_INET6);
        }

        uint32_t ipNetEndian() const;

        const uint32_t *ip6NetEndian() const;

        inline uint16_t portNetEndian() const
        {
            return addr_.sin_port;
        }

        void setPortNetEndian(uint16_t port)
        {
            addr_.sin_port = port;
        }

        static bool resolve(std::string& hostname, InetAddress* result);

        // set IPv6 ScopeID
        void setScopeId(uint32_t scope_id)
        {
            if (family() == AF_INET6)
            {
                addr6_.sin6_scope_id = scope_id;
            }
        }

    private:
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
        bool isIpV6_{false};
    };
}



#endif //DUODUO_INETADDRESS_H
