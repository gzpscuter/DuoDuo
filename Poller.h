//
// Created by gzp on 2021/3/4.
//

#ifndef DUODUO_POLLER_H
#define DUODUO_POLLER_H

#include "base/noncopyable.h"
#include "EventLoop.h"
using namespace base;

namespace duoduo {

    class Channel;

    class Poller : public noncopyable{
    public:
        explicit Poller(EventLoop *loop) : ownerLoop_(loop){};
        virtual ~Poller()
        {
        }
        void assertInLoopThread()
        {
            ownerLoop_->assertInLoopThread();
        }
        virtual void poll(int timeoutMs, ChannelList *activeChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        virtual void removeChannel(Channel *channel) = 0;
        virtual void resetAfterFork()
        {
        }
        static Poller *newPoller(EventLoop *loop);

    private:
        EventLoop *ownerLoop_;
    };
}



#endif //DUODUO_POLLER_H
