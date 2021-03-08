//
// Created by gzp on 2021/1/30.
//

#ifndef LONGADDR_DOUBLEADDR_H
#define LONGADDR_DOUBLEADDR_H

#include "MetaConcurrent.h"

namespace buffer {
    class DoubleAddr final : public MetaConcurrent<double>{
    public:
        DoubleAddr(){};
        ~DoubleAddr(){};

        void add(double val) {
            double b = 0;
            double v = 0;
            int m = 0;
            Cell<double>* a = nullptr;
            // when performing add-opt, add on base, if that failed, step into loop and search lowest-load cell
            if(m_cells != nullptr || !(casBase(b + val, b = m_base.load()))) {
                bool uncontened = true;
                if(m_cells == nullptr
                   || (m = nCells - 1) < 0  //purpose for this sentence is to avoid (a = as[-1 & -1]) in next sentence
                   || (a = m_cells[chosenIdx & m]) == nullptr
                   || !(uncontened = casCellValue(v + val, v = a->value.load(), a->value))){
                    metaAccumulate(val, uncontened);
                }
            }
        }

        double sum() {
            Cell<double>* a;
            double sum = m_base.load();
            if(m_cells != nullptr) {
                for(int i = 0; i < nCells; i++) {
                    if((a = m_cells[i]) != nullptr)
                        sum += a->value.load();
                }
            }
            return sum;
        }
    };

}


#endif //LONGADDR_DOUBLEADDR_H
