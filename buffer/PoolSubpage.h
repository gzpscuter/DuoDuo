//
// Created by gzp on 2021/2/1.
//

#ifndef LONGADDR_POOLSUBPAGE_H
#define LONGADDR_POOLSUBPAGE_H


#include <mutex>
#include <vector>
#include "commons.h"
#include "SizeClass.h"


namespace buffer {
    class PoolChunk;
    class PoolArena;

    class PoolSubpage {
        void addToPool(PoolSubpage* head);
        void removeFromPool();
        void setNextAvail(int bitmapIdx);
        int getNextAvail();
        int findNextAvail();
        int findNextAvail0(int i, long bits);
        long toHandle(int bitmapIdx);


        int m_pageShifts;
        int m_runOffset;
        int m_runSize;
        std::vector<long> m_bitmap;
        int m_maxNumElems;
        int m_bitmapLength;
        int m_nextAail;
        int m_numAvail;
    public:
        //default construction is used for head
        PoolSubpage();
        PoolSubpage(PoolSubpage * head, PoolChunk * chunk, int pageShifts, int runOffset, int runSize, int elemSize);

        bool free(PoolSubpage * head, int bitmapIdx);
        long allocate();


        PoolSubpage* prev;
        PoolSubpage* next;

        bool m_dontRelase;
        int m_elemSize;
        PoolChunk * m_chunk;

        std::mutex m_mtx;

    };

}


#endif //LONGADDR_POOLSUBPAGE_H


