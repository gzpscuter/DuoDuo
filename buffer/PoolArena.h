//
// Created by gzp on 2021/1/26.
//

#ifndef DUODUO_POOLARENA_H
#define DUODUO_POOLARENA_H

#include <atomic>
#include "commons.h"
#include "SizeClass.h"
#include "LongAddr/LongAddr.h"

namespace buffer{

    class PoolByteBufRecycler;
    class PoolByteBuf;
    class PoolByteBufAllocator;
    class SizeClass;
    class PoolSubpage;
    class PoolChunk;
    class PoolThreadCache;
    class DefaultArena;
    class PoolChunkList;



    class PoolArena: public SizeClass{

        std::vector<PoolSubpage*> m_smallSubpagePools;

        PoolChunkList* q50;
        PoolChunkList* q25;
        PoolChunkList* q00;
        PoolChunkList* qInit;
        PoolChunkList* q75;
        PoolChunkList* q100;

        long m_allocationsNormal;
        LongAddr m_allocationSmall;
        LongAddr m_allocationHuge;
        LongAddr m_activeBytesHuge;

        long m_deallocationSmall;
        long m_deallocationNormal;
        LongAddr m_deallocationHuge;

        std::mutex m_mtx;

//        inline SizeClassType getSizeClassType(long handle);
        PoolSubpage * newSubpagePoolHead();
        void allocate(PoolThreadCache* cache, PoolByteBuf* buf, int reqCapacity);
        void tcacheAllocateSmall(PoolThreadCache* cache, PoolByteBuf * buf, const int reqCapacity, const int sizeIdx);
        void tcacheAllocateNormal(PoolThreadCache* cache, PoolByteBuf * buf, const int reqCapacity, const int sizeIdx);
        void allocateNormal(PoolByteBuf * buf, int reqCapacity, int sizeIdx, PoolThreadCache* threadCache);
        void allocateHuge(PoolByteBuf * buf, int reqCapacity);

    public:
//        static enum SizeClassType {SMALL, NORMAL};
        PoolArena(PoolByteBufAllocator * parent, int pageSize,int pageShifts, int chunkSize);
        PoolByteBuf * allocate(PoolThreadCache* cache, int reqCapacity, int maxCapacity);
        void free(PoolChunk* chunk, const char * buffer, long handle, int normCapacity, PoolThreadCache* cache);
        SizeClassType getSizeClassType(long handle);
        void freeChunk(PoolChunk* chunk, long handle, int normCapacity, SizeClassType sizeClass, const char * buffer);
        PoolSubpage* findSubpagePoolHead(int sizeIdx);
        void reallocate(PoolByteBuf * buf, int newCapacity, bool freeOldMemory);

        virtual PoolChunk * newChunk(int pageSize, int maxPageIdx, int pageShifts, int chunkSize) = 0;
        virtual PoolChunk * newUnpoolChunk(int capacity) = 0;
        virtual PoolByteBuf * newByteBuf(int maxCapacity) = 0;
        virtual void memoryCopy(const char * memory, int srcOffset, PoolByteBuf * dstBuf, int length) = 0;
        virtual void destroyChunk(PoolChunk * chunk) = 0;

        int m_numSmallSubpagePools;
        PoolByteBufAllocator * m_parent;
        std::atomic<int> m_numThreadCaches;


    };


    class DefaultArena final : public PoolArena {
        PoolChunk * newChunk(int pageSize, int maxPageIdx, int pageShifts, int chunkSize) override;
        PoolChunk * newUnpoolChunk(int capacity) override;
        // new  char array[capacity]
        static char* defaultAllocate(int capacity);
        void destroyChunk(PoolChunk * chunk) override;
        PoolByteBuf * newByteBuf(int maxCapacity) override;
    public:

        DefaultArena(PoolByteBufAllocator * parent, int pageSize, int pageShifts, int chunkSize);
        void memoryCopy(const char* memory, int srcOffset, PoolByteBuf * dstBuf, int length) override;

    };

}


#endif //DUODUO_POOLARENA_H