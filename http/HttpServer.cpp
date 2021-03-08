//
// Created by gzp on 2021/3/6.
//

#include <iostream>
#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../EventLoop.h"
#include "../InetAddress.h"
#include "HttpContext.h"

using namespace std::placeholders;

namespace duoduo {
    namespace http {
        void defaultHttpCallback(const HttpRequest &, HttpResponse *resp) {
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }

        HttpServer::HttpServer(EventLoop *loop,
                               const InetAddress &listenAddr,
                               const string &name,
                               bool reusePort)
                : server_(loop, listenAddr, name, reusePort),
                  httpCallback_(defaultHttpCallback) {
            server_.setConnectionCallback(
                    std::bind(&HttpServer::onConnection, this, _1));
            server_.setRecvMessageCallback(
                    std::bind(&HttpServer::onMessage, this, _1, _2));
        }

        void HttpServer::start() {
            std::cout << "HttpServer[" << server_.name()
                     << "] starts listening on " /*<< server_.ipPort()*/;
            server_.start();
        }

        void HttpServer::onConnection(const TcpConnectionPtr &conn) {
            if (conn->connected()) {
                conn->setContext(HttpContext());
            }
        }

        void HttpServer::onMessage(const TcpConnectionPtr &conn,
                                   Buffer *buf) {
            HttpContext *context = any_cast<HttpContext>(conn->getMutableContext());

            if (!context->parseRequest(buf)) {
                conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
                conn->shutdown();
            }

            if (context->gotAll()) {
                onRequest(conn, context->request());
                context->reset();
            }
        }

        void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &req) {
            const string &connection = req.getHeader("Connection");
            bool close = connection == "close" ||
                         (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
            HttpResponse response(close);
            httpCallback_(req, &response);
            Buffer buf(16384);
            response.appendToBuffer(&buf);
            conn->send(&buf);
            if (response.closeConnection()) {
                conn->shutdown();
            }
        }
    }
}