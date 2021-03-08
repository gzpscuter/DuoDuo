//
// Created by gzp on 2021/3/4.
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "../TcpServer.h"
#include "../EventLoop.h"
#include "../InetAddress.h"
#include "../buffer/Buffer.h"
#include "../TcpConnection.h"


using namespace std;
using namespace duoduo;
using namespace std::placeholders;
using namespace buffer;


int numThreads = 0;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& listenAddr)
            : loop_(loop),
              server_(loop, listenAddr, "EchoServer")
    {
        server_.setConnectionCallback(
                std::bind(&EchoServer::onConnection, this, _1));
        server_.setRecvMessageCallback(
                std::bind(&EchoServer::onMessage, this, _1, _2));
        server_.setIoLoopNum(numThreads);
    }

    void start()
    {
        server_.start();
    }
    // void stop();

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        cout << conn->peerAddress().toIpPort() << " -> "
                  << conn->localAddress().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN") << endl;
//        cout << conn->getTcpInfoString();

        conn->send("hello\n");
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf)
    {
        string msg(buf->retrieveAllAsString());
//        LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at " << time.toString();
        if (msg == "exit\n")
        {
            conn->send("bye\n");
            conn->shutdown();
        }
        if (msg == "quit\n")
        {
            loop_->quit();
        }
        conn->send(msg);
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main(int argc, char* argv[])
{
//    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
//    LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);
    if (argc > 1)
    {
        numThreads = atoi(argv[1]);
    }
    numThreads = 4;
    bool ipv6 = argc > 2;
    EventLoop loop;
    InetAddress listenAddr(2000, false, false);
    EchoServer server(&loop, listenAddr);

    server.start();

    loop.loop();
}