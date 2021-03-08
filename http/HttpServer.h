//
// Created by gzp on 2021/3/6.
//

#ifndef DUODUO_HTTPSERVER_H
#define DUODUO_HTTPSERVER_H

#include <functional>
#include <cstring>
#include "../base/noncopyable.h"
#include "../../TcpConnection.h"
#include "../../TcpServer.h"
#include "../EventLoop.h"
#include "../InetAddress.h"
#include "../base/any.h"

using namespace base;
using namespace buffer;

namespace duoduo {
    namespace http {
        class HttpRequest;

        class HttpResponse;

//        class EventLoop;

//        class InetAddress;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
        class HttpServer : public noncopyable {
        public:
            typedef std::function<void(const HttpRequest &,
                                       HttpResponse *)> HttpCallback;

            HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const std::string &name,
                       bool reusePort = false);

            duoduo::EventLoop *getLoop() const { return server_.getLoop(); }

            /// Not thread safe, callback be registered before calling start().
            void setHttpCallback(const HttpCallback &cb) {
                httpCallback_ = cb;
            }

            void setThreadNum(int numThreads) {
                server_.setIoLoopNum(numThreads);
            }

            void start();

        private:
            void onConnection(const TcpConnectionPtr &conn);

            void onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf);

            void onRequest(const TcpConnectionPtr &, const HttpRequest &);

            TcpServer server_;
            HttpCallback httpCallback_;
        };
    }

}

#endif //DUODUO_HTTPSERVER_H
