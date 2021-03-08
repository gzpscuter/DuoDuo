//
// Created by gzp on 2021/2/28.
//

#ifndef DUODUO_ACCEPTOR_H
#define DUODUO_ACCEPTOR_H

#include <functional>
#include "base/noncopyable.h"
#include "Socket.h"
#include "Channel.h"
using namespace base;

namespace duoduo{

    class Acceptor : public noncopyable
    {
    public:
        typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

        Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback& cb)
        { newConnectionCallback_ = cb; }

        void listen();

    private:
        void handleRead();

        EventLoop* loop_;
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        int idleFd_;
    };

}


#endif //DUODUO_ACCEPTOR_H
