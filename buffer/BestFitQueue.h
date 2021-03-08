//
// Created by gzp on 2021/2/17.
//

#ifndef BUFFER_BESTFITQUEUE_H
#define BUFFER_BESTFITQUEUE_H
#include <vector>
#include <algorithm>
#include <queue>

namespace buffer {

    template<class T, class Sequence = std::vector<T>,
            class Compare = std::greater<typename Sequence::value_type> >
    class BestFitQueue{
    public:
        typedef typename Sequence::value_type value_type;
        typedef typename Sequence::size_type size_type;
        typedef typename Sequence::reference reference;
        typedef typename Sequence::const_reference const_reference;

    protected:
        Sequence c;
        Compare comp;

    public:
        BestFitQueue() : c() {}
        explicit BestFitQueue(const Compare& x) : c(), comp(x) {}

        template<class InputIterator>
        BestFitQueue(InputIterator first, InputIterator last)
        : c(first, last) {std::make_heap(c.begin(), c.end(), comp);}

        bool empty() {return c.empty();}
        size_type size() const {return c.size();}
        const_reference top() const {return c.front();}
        void push(const value_type& x) {
            try {
                c.push_back(x);
                std::push_heap(c.begin(), c.end(), comp);
            } catch (...) {
                c.clear();
                throw;
            }
        }

        void pop() {
            try {
                std::pop_heap(c.begin(), c.end(), comp);
                c.pop_back();
            } catch (...) {
                c.clear();
                throw;
            }
        }

        void remove(const value_type& x) {
            try {
                auto it_ = std::find(c.begin(), c.end(), x);
                if (it_ != c.end()) {
                    c.erase(it_);
                    std::make_heap(c.begin(), c.end(), comp);
                }
            } catch (...) {
                c.clear();
                throw;
            }
        }

    } ;

}


#endif //BUFFER_BESTFITQUEUE_H
