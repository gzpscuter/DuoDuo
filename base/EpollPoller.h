//
// Created by gzp on 2021/3/4.
//

#ifndef DUODUO_EPOLLPOLLER_H
#define DUODUO_EPOLLPOLLER_H

#include <memory>
#include <map>
#include <cstring>
#include "noncopyable.h"
#include "../Poller.h"
using namespace duoduo;

namespace base {


    class EpollPoller : public noncopyable,
                        public duoduo::Poller {
        typedef std::vector<struct epoll_event> EventList;
        using ChannelMap = std::map<int, Channel *>;

    public:

        explicit EpollPoller(EventLoop *loop);
        virtual ~EpollPoller();
        virtual void poll(int timeoutMs, ChannelList *activeChannels) override;
        virtual void updateChannel(Channel *channel) override;
        virtual void removeChannel(Channel *channel) override;

    private:

        static const int kInitEventListSize = 16;
        int epollfd_;
        EventList events_;
        void update(int operation, Channel *channel);
        ChannelMap channels_;
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    };
}



#endif //DUODUO_EPOLLPOLLER_H
