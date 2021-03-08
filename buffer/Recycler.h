//
// Created by gzp on 2021/2/7.
//

#ifndef LONGADDR_RECYCLER_H
#define LONGADDR_RECYCLER_H

#include <iostream>
#include <stack>
#include <mutex>
#include <numeric>
#include <assert.h>
#include "LongAddr/SpinLock.h"

namespace buffer {

    template<typename T>
    class Recycler{
        typedef  T value_type;

        static const int DEFAULT_INITIAL_MAX_CAPACITY_PER_THREAD = 4 * 1024;
        static const int DEFAULT_MAX_CAPACITY_PER_THREAD = DEFAULT_INITIAL_MAX_CAPACITY_PER_THREAD;
        static const int INITIAL_CAPACITY = 256;

        int m_maxCapacityPerThread;

        static thread_local std::stack<void *> tStack;
//        volatile static Recycler * m_instance;
        SpinLock m_spinLock;

//        Handle<T> * newHandle();

    public:
        //singleton mode
//        static Recycler* getInstance();

//        explicit Recycler(int maxCapacityPerThread);
        Recycler()
        : m_maxCapacityPerThread(DEFAULT_MAX_CAPACITY_PER_THREAD){
            assert(m_maxCapacityPerThread > 0);
        };
        Recycler(const Recycler & ) = delete;
        virtual ~Recycler() {};

        virtual value_type * newObject() = 0;

        inline value_type * get() {
            if(tStack.empty()) {
                value_type * m_value = newObject();
                return m_value;
            }
            value_type * objectItem = static_cast<value_type *>(tStack.top());
            tStack.pop();
            return objectItem;
        }

        inline bool recycle(value_type * o) {

            assert(o != nullptr);
            //因为wrk测试并发的时候，多个连接的多个Buffer送进此回收站，会超过INITIAL_CAPACITY，报错终止，所以注释了，实际中可以考虑打开这个限制
//            assert(tStack.size() <= INITIAL_CAPACITY);

            tStack.push(o);
            return true;
        }

    };


//    template<typename T>
//    class Handle {
////        int m_lastRecycledId;
////        int m_recycleId;
////        bool m_hasBeenRecycled;
//
//
//        std::stack<T *>* m_stack;
//    public:
//        Handle(std::stack<T *>* stack):m_stack(stack){};
//        Handle(const Handle& handle);
//
//        ~Handle();
//        std::stack<T *>* getStack();
//        void recycle(T * object);
//
//        T * m_value;
//    };

    template<typename T>
    thread_local std::stack<void *> Recycler<T>::tStack = std::stack<void *>();

}


#endif //LONGADDR_RECYCLER_H
