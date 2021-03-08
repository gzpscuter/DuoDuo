//
// Created by gzp on 2021/3/4.
//

#ifndef DUODUO_CALLBACKS_H
#define DUODUO_CALLBACKS_H
#include <functional>
#include "buffer/Buffer.h"

namespace duoduo {
    using TimerCallback = std::function<void()>;

// the data has been read to (buf, len)
    class TcpConnection;

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
// tcp server and connection callback
    using RecvMessageCallback = std::function<void(const TcpConnectionPtr &, buffer::Buffer *)>;
    using ConnectionErrorCallback = std::function<void()>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, const size_t)>;
//    using SSLErrorCallback = std::function<void(SSLError)>;

}

#endif //DUODUO_CALLBACKS_H
