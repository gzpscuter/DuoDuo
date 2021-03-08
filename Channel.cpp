//
// Created by zjw on 2021/1/10.
//

#include <poll.h>
#include "Channel.h"
#include "EventLoop.h"

namespace duoduo {


    Channel::Channel(EventLoop *loop, int fd)
            : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
    {
    }

    void Channel::remove()
    {
        assert(events_ == kNoneEvent);
        addedToLoop_ = false;
        loop_->removeChannel(this);
    }

    void Channel::update()
    {
        loop_->updateChannel(this);
    }

    void Channel::handleEvent()
    {
        if (tied_)
        {
            std::shared_ptr<void> guard = tie_.lock();
            if (guard)
            {
                handleEventWithGuard();
            }
        }
        else
        {
            handleEventWithGuard();
        }
    }

    void Channel::handleEventWithGuard()
    {
//        eventHandling_ = true;
//        LOG_TRACE << reventsToString();
        if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
        {
//            if (logHup_)
//            {
////                LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
//            }
            if (closeCallback_) closeCallback_();
        }

        if (revents_ & POLLNVAL)
        {
//            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
        }

        if (revents_ & (POLLERR | POLLNVAL))
        {
            if (errorCallback_) errorCallback_();
        }
        if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
        {
            if (readCallback_) readCallback_();
        }
        if (revents_ & POLLOUT)
        {
            if (writeCallback_) writeCallback_();
        }
//        eventHandling_ = false;
    }

}