//
// Created by gzp on 2021/3/1.
//

#ifndef DUODUO_EVENTLOOP_H
#define DUODUO_EVENTLOOP_H

#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>
#include <limits>
#include "base/noncopyable.h"
#include "MpscQueue.h"
#include "timer/Timer.h"

using namespace base;
using namespace timer;

namespace duoduo {
    class Channel;
    class Poller;

    typedef std::vector<Channel *> ChannelList;
    typedef std::function<void()> Func;

    class EventLoop : public noncopyable {
    public:
        EventLoop();
        ~EventLoop();

        /**
         * @brief Run the event loop. This method will be blocked until the event
         * loop exits.
         *
         */
        void loop();

        /**
         * @brief Let the event loop quit.
         *
         */
        void quit();

        /**
         * @brief Assertion that the current thread is the thread to which the event
         * loop belongs. If the assertion fails, the program aborts.
         */
        void assertInLoopThread()
        {
            if (!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        };

        void resetAfterFork();

        /**
         * @brief Return true if the current thread is the thread to which the event
         * loop belongs.
         *
         * @return true
         * @return false
         */
        bool isInLoopThread() const
        {
            return threadId_ == std::this_thread::get_id();
        };

        /**
         * @brief Get the event loop of the current thread. Return nullptr if there
         * is no event loop in the current thread.
         *
         * @return EventLoop*
         */
        static EventLoop *getEventLoopOfCurrentThread();

        /**
         * @brief Run the function f in the thread of the event loop.
         *
         * @param f
         * @note If the current thread is the thread of the event loop, the function
         * f is executed directly before the method exiting.
         */
        void runInLoop(const Func &f);

        /**
         * @brief Run the function f in the thread of the event loop.
         *
         * @param f
         * @note The difference between this method and the runInLoop() method is
         * that the function f is executed after the method exiting no matter if the
         * current thread is the thread of the event loop.
         */
        void queueInLoop(const Func &f);

        void runAt(const TimerExpiration &time, const Func &cb);
        void runAfter(const time_entry<>& delay, const Func &cb);
//        void runEvery(double interval, const Func &cb);

        /**
         * @brief Update channel status. This method is usually used internally.
         *
         * @param chl
         */
        void updateChannel(Channel *chl);

        /**
         * @brief Remove a channel from the event loop. This method is usually used
         * internally.
         *
         * @param chl
         */
        void removeChannel(Channel *chl);

        /**
         * @brief Return the index of the event loop.
         *
         * @return size_t
         */
        size_t index()
        {
            return index_;
        }

        /**
         * @brief Set the index of the event loop.
         *
         * @param index
         */
        void setIndex(size_t index)
        {
            index_ = index;
        }

    private:
        bool looping_;
        void abortNotInLoopThread();
        void wakeup();
        void handleRead();
        bool callingPendingFuncs_;
        std::thread::id threadId_;
        bool quit_;
        std::unique_ptr<Poller> poller_;

        ChannelList activeChannels_;
        Channel *currentActiveChannel_;

        bool eventHandling_;
        MpscQueue<Func> funcs_;
        std::unique_ptr<Timer> timer_;
        bool callingFuncs_{false};

        int wakeupFd_;
        std::unique_ptr<Channel> wakeupChannelPtr_;

        void doPendingFuncs();

        size_t index_{std::numeric_limits<size_t>::max()};

        EventLoop **threadLocalLoopPtr_;
    };

}


#endif //DUODUO_EVENTLOOP_H
