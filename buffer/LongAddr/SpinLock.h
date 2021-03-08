//
// Created by gzp on 2021/1/29.
//

#ifndef LONGADDR_SPINLOCK_H
#define LONGADDR_SPINLOCK_H


#include <atomic>
#include <sys/time.h>
#include <mutex>
#include <thread>

namespace buffer{
    class SpinLock
    {
        typedef size_t micro_seconds;

        std::atomic<bool> m_locked_flag = ATOMIC_VAR_INIT(false);
        micro_seconds m_SleepWhenAcquireFailedInMicroSeconds;

        SpinLock& operator=(const SpinLock) = delete;
        SpinLock(const SpinLock&) = delete;
    public:
        /// SpinLock的接口说明
        ///
        ///     @brief 该接口是SpinLock类的构造函数
        ///     @param _SleepWhenAcquireFailedInMicroSeconds 未抢到锁时线程阻塞的时间（微秒级），-1时表示不延时而直接让出时间片
        SpinLock(micro_seconds _SleepWhenAcquireFailedInMicroSeconds = micro_seconds(-1));
        /// lock的接口说明
        ///
        /// @brief 该接口通过原子变量m_locked_flag的cas操作实现线程互斥，供std::unique_lock构造函数调用
        void lock();

        void unlock();
    };
}


#endif //LONGADDR_SPINLOCK_H
