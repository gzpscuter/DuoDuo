//
// Created by gzp on 2021/1/25.
//

#ifndef DUODUO_POOLCHUNK_H
#define DUODUO_POOLCHUNK_H


#include <unordered_map>
#include <deque>
#include <mutex>
#include <memory>
#include "commons.h"
#include "BestFitQueue.h"


namespace buffer{
    class PoolArena;
    class DefaultArena;
    class PoolSubpage;
    class PoolByteBuf;
    class PoolThreadCache;
    class PoolByteBufAllocator;
    class PoolChunkList;

    class PoolChunk final{

        static const int SIZE_LENGTH_ = 15;
        static const int INUSED_LENGTH_ = 1;
        static const int SUBPAGE_LENGTH_ = 1;
        static const int BITMAP_IDX_LENGTH_ = 32;

        std::unordered_map<int, long> m_runsAvailMap;
        // m_runsAvail works in a bestFit way, so we need priority_queue
        //m_runsAvail由BestFitQueue指针组成，BestFitQueue存放大小相等的run区域，且offset最小的run区域优先
        std::vector<BestFitQueue<long>* > m_runsAvail;
        std::vector<PoolSubpage*> m_subpages;
//        PoolSubpage** m_subpages;
        std::deque<char *> m_cachedBuffers;
        std::mutex m_runAvailMtx;
        std::mutex m_subpageAllocMtx;

        int m_pageSize;
        int m_pageShifts;
        int m_chunkSize;

        std::vector<PoolSubpage*> newPoolSubpageArray(int size);
        std::vector<BestFitQueue<long>*> newRunsAvailQueue(int size);
        void insertAvailRun(int runOffset, int pages, long handle);
        inline static int tailPage(int runOffset, int pages);
        void insertAvailRun0(int runOffset, long handle);
        long allocateRun(int runSize);
        int  calculateRunSize(int sizeIdx);
        int runBestFit(int pageIdx);
        long splitLargeRun(long handle, int needPages);
        static long toRunHandle(int runOffset, int runPages, int inUsed);
        long allocateSubpage(int sizeIdx);
        long collapseRuns(long handle);
        long collapsePrev(long handle);
        long collapseNext(long handle);
        void removeAvailRun(long handle);
        void removeAvailRun(BestFitQueue<long>* bestFitQueue, long handle);
        int runSize(int pageShifts, long handle);
        long getAvailRunByOffset(int runOffset);
//        int usage(int freeBytes);


    public:
        static const int IS_SUBPAGE_SHIFT_ = BITMAP_IDX_LENGTH_;    //32
        static const int IS_INUSED_SHIFT_ = IS_SUBPAGE_SHIFT_ + SUBPAGE_LENGTH_;    //33
        static const int SIZE_SHIFT_ = IS_INUSED_SHIFT_ + INUSED_LENGTH_;       //34
        static const int RUN_OFFSET_SHIFT_ = SIZE_SHIFT_ + SIZE_LENGTH_;        //49

        PoolChunk(PoolArena* arena, const char * memory, int pageSize, int pageShifts, int chunkSize, int maxPageIdx, int offset);
        PoolChunk(PoolArena * arena, const char * memory, int size, int offset);
        ~PoolChunk();
        void free(long handle, int normCapacity, const char * buffer);
        void initBuf(PoolByteBuf* buf, const char * buffer, long handle, int reqCapacity, PoolThreadCache* threadCache);
        void initBufWithSubpage(PoolByteBuf * buf, const char * buffer, long handle, int reqCapacity, PoolThreadCache* threadCache);
        int calculateRunOffset(long handle);
        int calculateRunSize(int pageShifts, long handle);
        int calculateRunPages(long handle);
        static bool isUsed(long handle);
        static bool isRun(long handle);
        static bool isSubpage(long handle);
        int calculateBitmapIdx(long handle);
        int freeBytes();
        int chunkSize();
        bool allocate(PoolByteBuf * buf, int reqCapacity, int sizeIdx, PoolThreadCache * cache);
        int usage();


        PoolArena* m_arena;
        char * m_memory;
        int m_offset;
        int m_freeBytes;
        bool m_unpool;

        PoolChunk* prev;
        PoolChunk* next;
        PoolChunkList* m_parent;
    };

}


#endif //DUODUO_POOLCHUNK_H