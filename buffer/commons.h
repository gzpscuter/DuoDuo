//
// Created by gzp on 2021/2/17.
//

#ifndef BUFFER_COMMONS_H
#define BUFFER_COMMONS_H

namespace buffer {
    enum SizeClassType {SMALL, NORMAL};


    //封装一个函数对象(此处取消了模板化和继承)
    struct DeleteObject
    {
        template<typename T>
        void operator()(const T* ptr)const
        {
            if(ptr != nullptr) {
                delete ptr;
            }
        }
    };

}

#endif //BUFFER_COMMONS_H
