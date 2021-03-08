//
// Created by gzp on 2021/3/4.
//

#include "Poller.h"
#include "base/EpollPoller.h"

namespace duoduo {

    Poller *Poller::newPoller(EventLoop *loop)
    {
        return new EpollPoller(loop);
    }
}