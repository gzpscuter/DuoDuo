//
// Created by gzp on 2021/3/2.
//

#ifndef DUODUO_SOCKET_H
#define DUODUO_SOCKET_H

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>  // snprintf
#include "base/noncopyable.h"
#include "InetAddress.h"
using namespace base;

namespace duoduo{

    class Socket : public noncopyable {

    public:

        static void setNonBlockAndCloseOnExec(int sockfd)
        {
            // non-block
            int flags = ::fcntl(sockfd, F_GETFL, 0);
            flags |= O_NONBLOCK;
            int ret = ::fcntl(sockfd, F_SETFL, flags);
            // TODO check

            // close-on-exec
            flags = ::fcntl(sockfd, F_GETFD, 0);
            flags |= FD_CLOEXEC;
            ret = ::fcntl(sockfd, F_SETFD, flags);
            // TODO check

            (void)ret;
        }

        static int createNonblockingSocketOrDie(int family)
        {
            int sock = ::socket(family,
                            SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                            IPPROTO_TCP);
            if (sock < 0)
            {
//                LOG_SYSERR << "sockets::createNonblockingOrDie";
                exit(1);
            }
//            LOG_TRACE << "sock=" << sock;
            return sock;
        }

        static int getSocketError(int sockfd)
        {
            int optval;
            socklen_t optlen = static_cast<socklen_t>(sizeof optval);
            if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
            {
                return errno;
            }
            else
            {
                return optval;
            }
        }

        static int connect(int sockfd, const InetAddress &addr)
        {
            if (addr.isIpV6())
                return ::connect(sockfd,
                                 addr.getSockAddr(),
                                 static_cast<socklen_t>(
                                         sizeof(struct sockaddr_in6)));
            else
                return ::connect(sockfd,
                                 addr.getSockAddr(),
                                 static_cast<socklen_t>(
                                         sizeof(struct sockaddr_in)));
        }

        explicit Socket(int sockfd) : sockFd_(sockfd)
        {
        }
        ~Socket();
        /// abort if address in use
        void bindAddress(const InetAddress &localaddr);
        /// abort if address in use
        void listen();
        int accept(InetAddress *peeraddr);
        void closeWrite();
        int read(char *buffer, uint64_t len);
        int fd()
        {
            return sockFd_;
        }

        static struct sockaddr_in6 getLocalAddr(int sockfd);
        static struct sockaddr_in6 getPeerAddr(int sockfd);

        ///
        /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        ///
        void setTcpNoDelay(bool on);

        ///
        /// Enable/disable SO_REUSEADDR
        ///
        void setReuseAddr(bool on);

        ///
        /// Enable/disable SO_REUSEPORT
        ///
        void setReusePort(bool on);

        ///
        /// Enable/disable SO_KEEPALIVE
        ///
        void setKeepAlive(bool on);
        int getSocketError();
        static bool isSelfConnect(int sockfd);

    protected:
        int sockFd_;

    };
}



#endif //DUODUO_SOCKET_H
