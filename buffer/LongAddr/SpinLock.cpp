//
// Created by gzp on 2021/1/29.
//

#include "SpinLock.h"
namespace buffer {

    SpinLock::SpinLock(micro_seconds _SleepWhenAcquireFailedInMicroSeconds)
            : m_SleepWhenAcquireFailedInMicroSeconds(_SleepWhenAcquireFailedInMicroSeconds) {}

    void SpinLock::unlock() {
        m_locked_flag.store(false);
    }

    void SpinLock::lock() {
        bool exp = false;
        while (!m_locked_flag.compare_exchange_strong(exp, true)) {
            exp = false;
            if (m_SleepWhenAcquireFailedInMicroSeconds == micro_seconds(-1)) {
                std::this_thread::yield();
            } else if (m_SleepWhenAcquireFailedInMicroSeconds != 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(m_SleepWhenAcquireFailedInMicroSeconds));
            }
        }
    }

}