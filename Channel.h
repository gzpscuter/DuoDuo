//
// Created by zjw on 2021/1/10.
//

#ifndef DUODUO_CHANNEL_H
#define DUODUO_CHANNEL_H

#include <functional>
#include <memory>
#include <poll.h>
#include <assert.h>
#include "base/noncopyable.h"
#include "base/EpollPoller.h"
using namespace base;

namespace duoduo {
    class EventLoop;

    class Channel : public noncopyable {
    public:
        using EventCallback = std::function<void()>;
        /**
         * @brief Construct a new Channel instance.
         *
         * @param loop The event loop in which the channel works.
         * @param fd The socket fd.
         */
        Channel(EventLoop *loop, int fd);

        /**
         * @brief Set the read callback.
         *
         * @param cb The callback is called when read event occurs on the socket.
         * @note One should call the enableReading() method to ensure that the
         * callback would be called when some data is received on the socket.
         */
        void setReadCallback(const EventCallback &cb)
        {
            readCallback_ = cb;
        };
        void setReadCallback(EventCallback &&cb)
        {
            readCallback_ = std::move(cb);
        }

        void setWriteCallback(const EventCallback &cb)
        {
            writeCallback_ = cb;
        };
        void setWriteCallback(EventCallback &&cb)
        {
            writeCallback_ = std::move(cb);
        }

        /**
         * @brief Set the close callback.
         *
         * @param cb The callback is called when the socket is closed.
         */
        void setCloseCallback(const EventCallback &cb)
        {
            closeCallback_ = cb;
        }
        void setCloseCallback(EventCallback &&cb)
        {
            closeCallback_ = std::move(cb);
        }

        /**
         * @brief Set the error callback.
         *
         * @param cb The callback is called when an error occurs on the socket.
         */
        void setErrorCallback(const EventCallback &cb)
        {
            errorCallback_ = cb;
        }
        void setErrorCallback(EventCallback &&cb)
        {
            errorCallback_ = std::move(cb);
        }

        /**
         * @brief Set the event callback.
         *
         * @param cb The callback is called when any event occurs on the socket.
         * @note If the event callback is set to the channel, any other callback
         * wouldn't be called again.
         */
        void setEventCallback(const EventCallback &cb)
        {
            eventCallback_ = cb;
        }
        void setEventCallback(EventCallback &&cb)
        {
            eventCallback_ = std::move(cb);
        }

        /**
         * @brief Return the fd of the socket.
         *
         * @return int
         */
        int fd() const
        {
            return fd_;
        }

        /**
         * @brief Return the events enabled on the socket.
         *
         * @return int
         */
        int events() const
        {
            return events_;
        }

        /**
         * @brief Return the events that occurred on the socket.
         *
         * @return int
         */
        int revents() const
        {
            return revents_;
        }

        /**
         * @brief Check whether there is no event enabled on the socket.
         *
         * @return true
         * @return false
         */
        bool isNoneEvent() const
        {
            return events_ == kNoneEvent;
        };

        /**
         * @brief Disable all events on the socket.
         *
         */
        void disableAll()
        {
            events_ = kNoneEvent;
            update();
        }

        /**
         * @brief Remove the socket from the poller in the event loop.
         *
         */
        void remove();

        void handleEvent();

        EventLoop *ownerLoop()
        {
            return loop_;
        };

        void enableReading()
        {
            events_ |= kReadEvent;
            update();
        }

        void disableReading()
        {
            events_ &= ~kReadEvent;
            update();
        }

        void enableWriting()
        {
            events_ |= kWriteEvent;
            update();
        }

        void disableWriting()
        {
            events_ &= ~kWriteEvent;
            update();
        }

        bool isWriting() const
        {
            return events_ & kWriteEvent;
        }

        bool isReading() const
        {
            return events_ & kReadEvent;
        }

        void updateEvents(int events)
        {
            events_ = events;
            update();
        }

        /**
         * @brief This method is used to ensure that the callback owner is valid
         * when a callback is called.
         *
         * @param obj The callback owner. Usually, the owner is also the owner of
         * the channel.
         * @note The 'obj' is kept in a weak_ptr object, so this method does not
         * cause a circular reference problem.
         */
        void tie(const std::shared_ptr<void> &obj)
        {
            tie_ = obj;
            tied_ = true;
        }

        static const int kNoneEvent = 0;
        static const int kReadEvent = POLLIN | POLLPRI;
        static const int kWriteEvent = POLLOUT;


    private:
        friend class EventLoop;
        friend class base::EpollPoller;

        void update();
        void handleEventWithGuard();
        int setRevents(int revt)
        {
            // LOG_TRACE<<"revents="<<revt;
            revents_ = revt;
            return revt;
        };
        int index()
        {
            return index_;
        };
        void setIndex(int index)
        {
            index_ = index;
        };
        EventLoop *loop_;
        const int fd_;
        int events_;
        int revents_;
        int index_;
        bool addedToLoop_{false};
        EventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback errorCallback_;
        EventCallback closeCallback_;
        EventCallback eventCallback_;
        std::weak_ptr<void> tie_;
        bool tied_;
    };
}


#endif //DUODUO_CHANNEL_H
