//
// Created by gzp on 2021/2/11.
//

#ifndef LONGADDR_DEPENDENCE_H
#define LONGADDR_DEPENDENCE_H

#include <assert.h>
#include <unistd.h>

namespace base {
    class Dependence {
        static int NCPUS;
        Dependence() = delete;
        Dependence(const Dependence& ) = delete;
        static void normalize();
    public:
        static int processNum();

    };
}

#endif //LONGADDR_DEPENDENCE_H
