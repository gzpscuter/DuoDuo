//
// Created by gzp on 2021/2/7.
//

#include "Recycler.h"

namespace buffer {
//    template<typename T>
//    Recycler<T>::Recycler()
//    : m_maxCapacityPerThread(DEFAULT_MAX_CAPACITY_PER_THREAD){
//        assert(m_maxCapacityPerThread > 0);
//    };

//
//    template<typename T>
//    Recycler<T>* Recycler<T>::getInstance() {
//        if(m_instance == nullptr) {
//            std::unique_lock<SpinLock> lock(m_spinLock, std::defer_lock);
//            lock.lock();
//            if(m_instance == nullptr) {
//                m_instance = new Recycler();
//
//            }
//            lock.unlock();
//        }
//        return m_instance;
//    }

//    template<typename T>
//    volatile Recycler<T>* Recycler<T>::m_instance = nullptr;

//    template<typename T>
//    T * Recycler<T>::get() {
//        if(tStack.empty()) {
////            Handle<T>* handle = newHandle();
//            T * m_value = newObject();
//            return m_value;
////            return static_cast<T>(handle->m_value);
//        }
//        T * objectItem = tStack.top();
//        tStack.pop();
////        if(handle == nullptr) {
////            handle = newHandle();
////            handle->m_value = newObject(handle);
////
////        }
//        return objectItem;
//    }

//    template<typename T>
//    bool Recycler<T>::recycle(T * o) {
//        assert(o != nullptr);
//        assert(tStack.size() <= INITIAL_CAPACITY);
//
////        if(handle->stack()->parent != this)
////            return false;
//        tStack.push(o);
//        return true;
//    }

//    template<typename T>
//    Handle<T> * Recycler<T>::newHandle() {
//        return new Handle<T>(&tStack);
//    }

//    template<typename T>
//    T* Recycler<T>::newObject(Handle<T>* handle) {
//        return new T(handle);
//    }

//class Handle
//    template<typename T>
//    Handle<T>::Handle(const Handle& handle) {
//        m_stack = handle.m_stack;
//        m_value = handle.m_value;
//    }
//
//    template<typename T>
//    std::stack<T>* Handle<T>::getStack(){
//        return m_stack;
//    }
//
//    template<typename T>
//    void Handle<T>::recycle(T * object) {
//        assert(m_value == object);
////            static_assert(m_lastRecycledId == m_recycleId && m_stack != nullptr, "recycled already");
//        m_stack->push(this);
//    }

}