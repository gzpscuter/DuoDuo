//
// Created by gzp on 2021/3/2.
//

#ifndef DUODUO_EVENTLOOPTHREAD_H
#define DUODUO_EVENTLOOPTHREAD_H

#include <mutex>
#include <thread>
#include <memory>
//#include <condition_variable>
#include <future>
#include <functional>
#include "base/noncopyable.h"
using namespace base;

namespace duoduo {
    class EventLoop;
    class EventLoopThread : public noncopyable {
    public:
        explicit EventLoopThread(const std::string &threadName = "EventLoopThread");
        ~EventLoopThread();

        /**
         * @brief Wait for the event loop to exit.
         * @note This method blocks the current thread until the event loop exits.
         */
        void wait();

        /**
         * @brief Get the pointer of the event loop of the thread.
         *
         * @return EventLoop*
         */
        EventLoop *getLoop() const
        {
            return loop_;
        }

        /**
         * @brief Run the event loop of the thread. This method doesn't block the
         * current thread.
         *
         */
        void run();

    private:
        EventLoop *loop_;
        std::string loopThreadName_;
        void loopFuncs();
        std::promise<EventLoop *> promiseForLoopPointer_;
        std::promise<int> promiseForRun_;
        std::promise<int> promiseForLoop_;
        std::once_flag once_;
        std::thread thread_;
    };

}


#endif //DUODUO_EVENTLOOPTHREAD_H
