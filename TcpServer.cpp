//
// Created by gzp on 2021/3/4.
//

#include <iostream>
#include "TcpServer.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "Socket.h"
#include "buffer/Buffer.h"

using namespace std::placeholders;
namespace duoduo {

    TcpServer::TcpServer(EventLoop *loop,
                         const InetAddress &address,
                         const std::string &name,
                         bool reUsePort)
            : loop_(loop),
              acceptorPtr_(new Acceptor(loop, address, reUsePort)),
              serverName_(name),
              recvMessageCallback_([](const TcpConnectionPtr &, buffer::Buffer *buffer) {
                  std::cout << "unhandled recv message [" << buffer->readableBytes()
                            << " bytes]";
                  buffer->retrieveAll();
              })
    {
        acceptorPtr_->setNewConnectionCallback(
                std::bind(&TcpServer::newConnection, this, _1, _2));
    }

    TcpServer::~TcpServer() {
        loop_->assertInLoopThread();
        loop_->runInLoop([this]() { acceptorPtr_.reset(); });
        for (auto& connection : connSet_)
        {
            connection->forceClose();
        }
        loopPoolPtr_.reset();
    }

    void TcpServer::start()
    {
        static std::once_flag flag;
        std::call_once(flag, [this] {
            loopPoolPtr_->start();

            loop_->runInLoop(
                    std::bind(&Acceptor::listen, acceptorPtr_.get()));
        });
    }

    void TcpServer::newConnection(int sockfd, const InetAddress &peer)
    {
        std::cout << "new connection:fd=" << sockfd
                  << " address=" << peer.toIpPort();

        loop_->assertInLoopThread();
        EventLoop *ioLoop = nullptr;
        if (loopPoolPtr_ && loopPoolPtr_->size() > 0)
        {
            ioLoop = loopPoolPtr_->getNextLoop();
        }
        if (ioLoop == nullptr)
            ioLoop = loop_;
        std::shared_ptr<TcpConnection> newPtr(
                new TcpConnection(ioLoop, sockfd, InetAddress(Socket::getLocalAddr(sockfd)), peer )
                );

        newPtr->setMessageCallback(recvMessageCallback_);

        newPtr->setConnectionCallback(
                [this](const TcpConnectionPtr &connectionPtr) {
                    if (connectionCallback_)
                        connectionCallback_(connectionPtr);
                });
        newPtr->setWriteCompleteCallback(
                [this](const TcpConnectionPtr &connectionPtr) {
                    if (writeCompleteCallback_)
                        writeCompleteCallback_(connectionPtr);
                });
        newPtr->setCloseCallback(std::bind(&TcpServer::connectionClosed, this, _1));
        connSet_.insert(newPtr);
        ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, newPtr));
//        newPtr->connectEstablished();
    }

    void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr)
    {
        std::cout << "connectionClosed";
        // loop_->assertInLoopThread();
        loop_->runInLoop([this, connectionPtr]() {
            size_t n = connSet_.erase(connectionPtr);
            (void)n;
            assert(n == 1);
        });

        static_cast<TcpConnection *>(connectionPtr.get())->connectDestroyed();
    }
}