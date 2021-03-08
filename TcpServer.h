//
// Created by gzp on 2021/3/4.
//

#ifndef DUODUO_TCPSERVER_H
#define DUODUO_TCPSERVER_H

#include <string>
#include <memory>
#include <set>
#include <assert.h>
#include <vector>
#include "base/noncopyable.h"
#include "EventLoopThreadPool.h"
#include "callbacks.h"
using namespace base;

namespace duoduo {
    class EventLoop;
    class InetAddress;
    class Acceptor;

    class TcpServer : public noncopyable {
    public:
        TcpServer(EventLoop *loop,
                  const InetAddress &address,
                  const std::string &name,
                  bool reUsePort = false);
        ~TcpServer();

        void start();

        void setIoLoopNum(size_t num)
        {
            assert(!started_);
//            loopPoolPtr_ = std::make_shared<EventLoopThreadPool>(num);
            loopPoolPtr_.reset(new EventLoopThreadPool(num));
            loopPoolPtr_->start();
        }

        /**
         * @brief Set the message callback.
         *
         * @param cb The callback is called when some data is received on a
         * connection to the server.
         */
        void setRecvMessageCallback(const RecvMessageCallback &cb)
        {
            recvMessageCallback_ = cb;
        }

        /**
         * @brief Set the connection callback.
         *
         * @param cb The callback is called when a connection is established or
         * closed.
         */
        void setConnectionCallback(const ConnectionCallback &cb)
        {
            connectionCallback_ = cb;
        }

        /**
         * @brief Set the write complete callback.
         *
         * @param cb The callback is called when data to send is written to the
         * socket of a connection.
         */
        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            writeCompleteCallback_ = cb;
        }

        const std::string &name() const
        {
            return serverName_;
        }

        /**
         * @brief Get the acceptor loop of the server.
         *
         * @return EventLoop*
         */
        EventLoop *getLoop() const
        {
            return loop_;
        }

        /**
         * @brief Get the I/O event loops of the server.
         *
         * @return std::vector<EventLoop *>
         */
        std::vector<EventLoop *> getIoLoops() const
        {
            return loopPoolPtr_->getLoops();
        }


    private:
        EventLoop *loop_;   // the acceptor loop
        std::unique_ptr<Acceptor> acceptorPtr_;
        void newConnection(int fd, const InetAddress &peer);
        std::string serverName_;
        std::set<TcpConnectionPtr> connSet_;

        RecvMessageCallback recvMessageCallback_;
        ConnectionCallback connectionCallback_;
        WriteCompleteCallback writeCompleteCallback_;

//        std::map<EventLoop *, std::shared_ptr<TimingWheel>> timingWheelMap_;
        void connectionClosed(const TcpConnectionPtr &connectionPtr);
        std::shared_ptr<EventLoopThreadPool> loopPoolPtr_;

        bool started_{false};
    };

}


#endif //DUODUO_TCPSERVER_H
