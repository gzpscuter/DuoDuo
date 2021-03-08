//
// Created by gzp on 2021/2/28.
//

#ifndef DUODUO_NONCOPYABLE_H
#define DUODUO_NONCOPYABLE_H


namespace base
{

    class noncopyable
    {
    public:
        noncopyable(const noncopyable&) = delete;
        void operator=(const noncopyable&) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };

}  // namespace muduo


#endif //DUODUO_NONCOPYABLE_H
