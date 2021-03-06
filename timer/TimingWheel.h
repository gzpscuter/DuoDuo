/**
 *
 *  TimingWheel.cc
 *  gzp-amateur
 *
 *  Copyright 2020, gzp-amateur.  All rights reserved.
 *  https://github.com/gzpscuter/reposit/Timer
 *
 *  Timer
 *
 */

#pragma once

#include "delay_queue/DelayQueue.h"
#include "delay_queue/TimeEntry.h"
#include <atomic>
#include <vector>
#include <memory>



namespace timer {
    class TimerTask;
    class TimerTaskEntry;
    class TimerTaskList;
    class DelayQueue;
    
    class TimingWheel {
    public:
        TimingWheel(const time_entry<>& tickMs, int wheelSize, const time_entry<>&,
                    std::shared_ptr< std::atomic<int> >& taskCounter, std::shared_ptr<delay_queue::DelayQueue<TimerTaskList> >& queue);
        ~TimingWheel();
        void advanceClock(const time_entry<>& timeMs);
        bool add(TimerTaskEntry* TimerTaskEntry);

    private:
        time_entry<> m_tickMs;
        int m_wheelSize;
        time_entry<> startMs;
        std::shared_ptr< std::atomic<int> > m_taskCounter;
        std::shared_ptr< delay_queue::DelayQueue<TimerTaskList> > m_queue;

        time_entry<> m_interval;
        std::vector<TimerTaskList> m_buckets;
        time_entry<> m_currentTime;
        TimingWheel* m_overflowWheel;
        std::mutex m_mutex;

        void addOverflowWheel();
        

    };
}
