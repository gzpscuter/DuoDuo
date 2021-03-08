//
// Created by gzp on 2021/3/2.
//

#include <sys/prctl.h>
#include "EventLoopThread.h"
#include "EventLoop.h"

namespace duoduo {

    EventLoopThread::EventLoopThread(const std::string &threadName)
            : loop_(nullptr),
              loopThreadName_(threadName),
              thread_(std::bind(&EventLoopThread::loopFuncs, this))
    {
        auto f = promiseForLoopPointer_.get_future();
        loop_ = f.get();
    }
    EventLoopThread::~EventLoopThread()
    {
        run();
        if (loop_)
        {
            loop_->quit();
        }
        if (thread_.joinable())
        {
            thread_.join();
        }
    }
// void EventLoopThread::stop() {
//    if(loop_)
//        loop_->quit();
//}
    void EventLoopThread::wait()
    {
        thread_.join();
    }

    void EventLoopThread::loopFuncs()
    {
        ::prctl(PR_SET_NAME, loopThreadName_.c_str());
        EventLoop loop;
        loop.queueInLoop([this]() { promiseForLoop_.set_value(1); });
        promiseForLoopPointer_.set_value(&loop);
        auto f = promiseForRun_.get_future();
        (void)f.get();
        loop.loop();
        // LOG_DEBUG << "loop out";
        loop_ = NULL;
    }

    void EventLoopThread::run()
    {
        std::call_once(once_, [this]() {
            auto f = promiseForLoop_.get_future();
            promiseForRun_.set_value(1);
            // Make sure the event loop loops before returning.
            (void)f.get();
        });
    }
}