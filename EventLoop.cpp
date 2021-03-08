//
// Created by gzp on 2021/3/1.
//

#include <iostream>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include "timer/delay_queue/TimeEntry.h"

namespace duoduo {

    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            std::cout << "Failed in eventfd" << std::endl;
            abort();
        }

        return evtfd;
    }
    const int kPollTimeMs = 10000;

    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
            // LOG_TRACE << "Ignore SIGPIPE";
        }
    };
    IgnoreSigPipe initObj;

    thread_local EventLoop *t_loopInThisThread = nullptr;

    EventLoop::EventLoop()
            : looping_(false),
              threadId_(std::this_thread::get_id()),
              quit_(false),
              poller_(Poller::newPoller(this)),
              currentActiveChannel_(nullptr),
              eventHandling_(false),
              timer_(Timer::newTimer(time_entry<>({0, 200000}), 20)),
              threadLocalLoopPtr_(&t_loopInThisThread),
              wakeupFd_(createEventfd()),
              wakeupChannelPtr_(new Channel(this, wakeupFd_))
    {
        if (t_loopInThisThread) {
            std::cout << "There is already an EventLoop in this thread";
            exit(-1);
        }
        t_loopInThisThread = this;
        wakeupChannelPtr_->setReadCallback(std::bind(&EventLoop::handleRead, this));
        wakeupChannelPtr_->enableReading();
    }

    void EventLoop::resetAfterFork()
    {
        poller_->resetAfterFork();
    }

    EventLoop::~EventLoop()
    {
        assert(!looping_);
        t_loopInThisThread = nullptr;
        ::close(wakeupFd_);
    }

    EventLoop *EventLoop::getEventLoopOfCurrentThread()
    {
        return t_loopInThisThread;
    }

    void EventLoop::runAt(const TimerExpiration &time, const Func &cb)
    {
        timer_->addByExpiration(time, cb);
    }

    void EventLoop::runAfter(const time_entry<>& delay, const Func &cb)
    {
        timer_->addByDelay(delay, cb);
    }
//    void EventLoop::runEvery(double interval, const Func &cb);

    void EventLoop::updateChannel(Channel *channel)
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        poller_->updateChannel(channel);
    }

    void EventLoop::removeChannel(Channel *channel)
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        if (eventHandling_)
        {
            assert(currentActiveChannel_ == channel ||
                   std::find(activeChannels_.begin(),
                             activeChannels_.end(),
                             channel) == activeChannels_.end());
        }
        poller_->removeChannel(channel);
    }

    void EventLoop::quit()
    {
        quit_ = true;
        // There is a chance that loop() just executes while(!quit_) and exits,
        // then EventLoop destructs, then we are accessing an invalid object.
        // Can be fixed using mutex_ in both places.
        if (!isInLoopThread())
        {
            wakeup();
        }
    }

    void EventLoop::loop()
    {
        assert(!looping_);
        assertInLoopThread();
        looping_ = true;
        quit_ = false;

        while (!quit_)
        {
            activeChannels_.clear();

            poller_->poll(kPollTimeMs, &activeChannels_);

            eventHandling_ = true;
            for (auto it = activeChannels_.begin(); it != activeChannels_.end();
                 ++it)
            {
                currentActiveChannel_ = *it;
                currentActiveChannel_->handleEvent();
            }
            currentActiveChannel_ = nullptr;
            eventHandling_ = false;
            // std::cout << "looping" << endl;
            doPendingFuncs();
        }
        looping_ = false;
    }

    void EventLoop::abortNotInLoopThread()
    {
        quit_ = true;
        std::cout << "It is forbidden to run loop on threads other than event-loop "
                     "thread";
        exit(1);
    }

    void EventLoop::runInLoop(const Func &cb)
    {
        if (isInLoopThread())
        {
            cb();
        }
        else
        {
            queueInLoop(cb);
        }
    }

    void EventLoop::queueInLoop(const Func &cb)
    {
        funcs_.enqueue(cb);
        if (!isInLoopThread() || !callingPendingFuncs_)
        {
            wakeup();
        }
    }

    void EventLoop::wakeup()
    {
        // if (!looping_)
        //     return;
        uint64_t tmp = 1;
        ssize_t ret = ::write(wakeupFd_, &tmp, sizeof(tmp));
    }

    void EventLoop::handleRead()
    {
        ssize_t ret = 0;
        uint64_t tmp;
        ret = read(wakeupFd_, &tmp, sizeof(tmp));
        if (ret < 0)
            std::cout << "wakeup read error";
    }

    void EventLoop::doPendingFuncs()
    {
        callingPendingFuncs_ = true;
        {
            // the destructor for the Func may itself insert a new entry into the
            // queue
            while (!funcs_.empty())
            {
                Func func;
                while (funcs_.dequeue(func))
                {
                    func();
                }
            }
        }
        callingPendingFuncs_ = false;
    }

}