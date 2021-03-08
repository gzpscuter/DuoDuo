//
// Created by gzp on 2021/1/27.
//

#ifndef DUODUO_METACONCURRENT_H
#define DUODUO_METACONCURRENT_H

#include <vector>
#include <atomic>
#include <assert.h>
#include <unistd.h>
#include <mutex>
#include "SpinLock.h"


namespace buffer{

    template<class T>
    struct Cell {
        typedef unsigned int uint32_t;

        Cell()
        {
            value.store(0);
        }

        explicit Cell(T val) {
            value.store(val);
        }
        ~Cell(){};
        Cell(const Cell&) = delete;
        Cell& operator = (const Cell&) = delete;

        volatile uint32_t prev;
        volatile std::atomic<T> value;  // use volatile to avoid instruction resort, thus avoid triggering cacheline mechanism
        uint32_t next;
    };

    extern thread_local int chosenIdx;


    template<class T>
    class MetaConcurrent {
        // this CallBack structor is triggered when cpp-thread destroyed
        struct CallBack {
            std::atomic<int>* _nth;
            CallBack(std::atomic<int>* bindNth) {
                _nth = bindNth;
            }
            ~CallBack() {
                (*_nth)--;
            }
        };

        SpinLock m_spinLock;

    protected:
        int NCPUS = static_cast<int>(sysconf(_SC_NPROCESSORS_CONF));

        std::atomic<T> m_base;
        std::atomic<bool> m_cellBusy = ATOMIC_VAR_INIT(false);
        Cell<T> ** m_cells;
        int nCells;
        std::atomic<int> *m_nthArray;

        MetaConcurrent()
        :m_cells(nullptr),
        m_nthArray(nullptr),
        m_base(0),
        nCells(0)
        {
            // initialize m_nthArray and avoid its extending, for callback-fn need to find m_nthArray
            assert (NCPUS > 0);
            NCPUS --;
            NCPUS |= NCPUS >> 1;
            NCPUS |= NCPUS >> 2;
            NCPUS |= NCPUS >> 4;
            NCPUS |= NCPUS >> 8;
            NCPUS |= NCPUS >> 16;
            NCPUS ++;
            m_nthArray = new std::atomic<int>[NCPUS];
            for(int i = 0; i < NCPUS; i++)
                m_nthArray[i] = 0;
        }

        ~MetaConcurrent() {
            if(nullptr != m_cells) {
                for(int i = 0; i < nCells; i++){
                    m_cells[i] = nullptr;
                }
                delete[] m_cells;
                m_cells = nullptr;
            }

            if(nullptr != m_nthArray) {
                delete[] m_nthArray;
                m_nthArray = nullptr;
            }
        }

        inline void freeCells(Cell<T>** cells, int num) {
            if(nullptr != cells) {
                for(int i = 0; i < num; i++){
                    cells[i] = nullptr;
                }
                delete[] cells;
                cells = nullptr;
            }
        }

        // need to reverse the fed parameters, for c++ feed params in a right_to_left way
        inline bool casBase(T desire, T& expected) {
            return m_base.compare_exchange_weak(expected, desire);
        }

        inline bool casCellValue(T desire, T value, volatile std::atomic<T>& expected) {
            return expected.compare_exchange_weak(value, desire);
        }

        inline bool casCellBusy() {
            bool exp = false;
            return m_cellBusy.compare_exchange_weak(exp, true);
        }

        // load balance algorithm for finding cell with the least load
        void chooseCell() {
            std::unique_lock<SpinLock> lock(m_spinLock);
            if(nCells > 1) {
                for(int i = 0; i < nCells; i++ ) {
                    if(m_cells[i] == nullptr && m_nthArray[i] == 0) {
                        chosenIdx = i;
                        if(chosenIdx == -1)
                            chosenIdx = nCells - 1;
                        break;
                    }
                    else {
                        if (m_nthArray[i] <= m_nthArray[chosenIdx & nCells - 1])
                            chosenIdx = i;
                    }
                }
            } else {
                chosenIdx = 0;
            }

            // chosenIdx points to slot having 0 or the least nthreads
            if(m_cells != nullptr)
                m_nthArray[chosenIdx & nCells - 1]++;
        }

        // must confirm this is the main entry for add-opt
        void metaAccumulate(T val, bool wasUncontended)
        {
            if(m_cells != nullptr && chosenIdx == -1) {
                chooseCell();
                static thread_local CallBack cb(&m_nthArray[chosenIdx & nCells - 1]);
            }

            bool extend = false;
//            std::cout <<"binding threads : ";
//            for(int i = 0; i < nCells; i++) {
//                std::cout << m_nthArray[i]<< " ";
//            }
//            std::cout << std::endl;

            for(;;) {
                Cell<T> ** as = nullptr; Cell<T>* a = nullptr; int n = 0; T v;
                if((as = m_cells) != nullptr && (n = nCells) > 0) {
                    if((a = as[chosenIdx & n - 1]) == nullptr) {
                        if(!m_cellBusy ) {
                            Cell<T>* r = new Cell<T>(val);
                            if(!m_cellBusy && casCellBusy()) {
                                bool created = false;

                                    int m = 0, j = 0;
                                    if(m_cells != nullptr && (m = nCells) > 0 &&
                                        m_cells[j = chosenIdx & m - 1] == nullptr) {
                                        m_cells[j] = r;
                                        created = true;
                                    }

                                bool ret = true;
                                m_cellBusy.compare_exchange_strong(ret, false);

                                if(created) break;
                                continue;
                            }
                        }
                        extend = false;
                    }
                    else if(!wasUncontended)
                        wasUncontended = true;
                    else if(casCellValue(v + val, v = a->value.load(), a->value))
                        break;
                    else if(n >= NCPUS || m_cells != as)
                        extend = false;
                    else if(!extend)
                        extend = true;
                    else if(!m_cellBusy && casCellBusy()) {

                            if(m_cells == as) {
                                auto rs = new Cell<T>*[n << 1];

                                for(int i = 0; i < n; i++) {
                                    rs[i] = as[i];
                                }
                                // warning warning warning warning warning !!!!!!!!!!!  see here !!!!!!!!!!!!!!!!!!
                                //
                                // in multi-thread case, to assign an array-pointer @param m_cells to a new address, we need to save the old address by
                                // definition another pointer pointing to the old address and then make assignment. note that if we delete @param m_cells
                                // first and then assign @param m_cells to the new address, within the duration of delete and assign of @param m_cells,
                                // other threads may get @param m_cells == nullptr, so unexpected cases may occur.
                                Cell<T>** toDel = m_cells;
                                m_cells = rs;
                                freeCells(toDel, n);
                                rs = nullptr;
                                nCells = n << 1;
                            }

                        bool ret = true;
                        m_cellBusy.compare_exchange_strong(ret, false);

                        extend = false;
                        continue;
                    }
                }
                else if(!m_cellBusy && m_cells == as && casCellBusy()) {
                    bool init = false;

                        if (m_cells == as) {
                            Cell<T>** rs = new Cell<T>*[2];
//                            std::atomic<int>* nths = new std::atomic<int>[2];
                            nCells = 2;
                            rs[chosenIdx & nCells - 1] = new Cell<T>(val);
                            m_nthArray[chosenIdx & nCells - 1]++;
//                            rs[0] = new Cell<T>();
                            chosenIdx &= nCells - 1;
                            static thread_local CallBack cb(&m_nthArray[chosenIdx & nCells - 1]);
                            m_cells = rs;
                            rs = nullptr;
                            init = true;
                        }

                    bool ret = true;
                    m_cellBusy.compare_exchange_strong(ret, false);

                    if(init)
                        break;

                }
                else if(casBase( v + val, v = m_base.load()))
                    break;
            }
        }
    };

}


#endif //DUODUO_METACONCURRENT_H