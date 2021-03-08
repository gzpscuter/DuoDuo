//
// Created by gzp on 2021/2/11.
//
#include "Dependence.h"
namespace base {
    int Dependence::NCPUS = static_cast<int>(sysconf(_SC_NPROCESSORS_CONF));

    int Dependence::processNum() {
        if(NCPUS == 0)
            normalize();
        return NCPUS;
    }

    void Dependence::normalize() {
        NCPUS = static_cast<int>(sysconf(_SC_NPROCESSORS_CONF));
        assert (NCPUS > 0);
        NCPUS --;
        NCPUS |= NCPUS >> 1;
        NCPUS |= NCPUS >> 2;
        NCPUS |= NCPUS >> 4;
        NCPUS |= NCPUS >> 8;
        NCPUS |= NCPUS >> 16;
        NCPUS ++;
    }

}