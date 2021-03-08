//
// Created by gzp on 2021/2/2.
//

#ifndef LONGADDR_POOLTHREADCACHE_H
#define LONGADDR_POOLTHREADCACHE_H

#include <exception>
#include <list>
#include <atomic>
#include <queue>
#include <numeric>
#include <assert.h>
#include "commons.h"
#include "Recycler.h"
#include "SizeClass.h"
#include "MathUtils.h"


namespace buffer {
    class PoolArena;
    class MemoryRegionCache;
    class SubpageMemoryRegionCache;
    class NormalMemoryRegionCache;
    class PoolChunk;
    class PoolByteBuf;

    class PoolThreadCache final {
    public:

        PoolThreadCache(PoolArena* arena, int smallCacheSize,
                                         int normalCacheSize, int maxCacheCapaicy, int freeSweepAllocationThreshold);
        ~PoolThreadCache();
        bool allocateSmall(PoolArena* arena, PoolByteBuf * buf, int reqCapacity, int sizeIdx);
        bool allocateNormal(PoolArena* arena, PoolByteBuf * buf, int reqCapacity, int sizeIdx);
        bool add(PoolArena* arena, PoolChunk* chunk, const char * buffer, long handle, int normCapacity, SizeClassType sizeClass);
        void free();
        void trim();

        inline MemoryRegionCache * cache(std::vector<MemoryRegionCache*> & cache, int sizeIdx) {
            if (cache.empty() || sizeIdx >= cache.size()) {
                return nullptr;
            }
            return cache[sizeIdx];
        }


        PoolArena* m_arena;
    private:

        std::vector<MemoryRegionCache*> createSubpageCaches(int cacheSize, int numCaches);
        std::vector<MemoryRegionCache*> createNormalCaches(int cacheSize, int maxCacheCapacity, PoolArena* arena);
        bool allocate(MemoryRegionCache* cache, PoolByteBuf * buf, int reqCapacity);
        MemoryRegionCache* genCache(PoolArena* arena, int sizeIdx, SizeClassType sizeClass);
        int free(std::vector<MemoryRegionCache*> & caches);
        int free(MemoryRegionCache* cache);
        void trim(std::vector<MemoryRegionCache*> & caches);
        void trim(MemoryRegionCache* cache);
        MemoryRegionCache * cacheForSmall(PoolArena* arena, int sizeIdx);
        MemoryRegionCache * cacheForNormal(PoolArena * arena, int sizeIdx);


        std::vector<MemoryRegionCache* > m_smallSubpageCaches;
        std::vector<MemoryRegionCache* > m_normalCaches;

//        int m_lruTriggerThreshold;
        std::atomic_bool m_toFree = ATOMIC_VAR_INIT(false);
        int m_allocations;
        int m_freeSweepAllocationThreshold;

    };


    class MemoryRegionCache {
        class EntryBufRecycler;

        struct Entry {
//            Handle<Entry>* recyclerHanle;
            static Recycler<Entry> * m_recycler;
            PoolChunk* chunk;
            char * buffer;
            long handle;
            int normCapacity;

            Entry()
            : chunk(nullptr),
            buffer(nullptr),
            handle(-1L),
            normCapacity(0) {
//                this->recyclerHandler = recyclerHandler;
            }

//            Entry(const Entry& entry) {
////                recyclerHanle = entry.recyclerHanle;
//                chunk = entry.chunk;
//                handle = entry.handle;
//                normCapacity = entry.normCapacity;
//                buffer = entry.buffer;
//            }

            ~Entry() {
//                if(nullptr != recyclerHanle) delete recyclerHanle;
                if(nullptr != chunk) delete chunk;
                if(nullptr != buffer) delete[] buffer;

            }

            void recycler() {
                this->chunk = nullptr;
                this->buffer = nullptr;
                this->handle = -1;
                m_recycler->recycle(this);
            }


        };

        class EntryBufRecycler : public Recycler<Entry> {
            Entry * newObject() {
                return new Entry();
            }
        };

        int m_size;
        SizeClassType m_sizeClassType;
        int m_allocations;
//        Recycler<Entry>* m_recycler;

        int free(int freeNum);
        void freeEntry(Entry* entry);
        Entry * newEntry(PoolChunk* chunk, const char * buffer, long handle, int normCapacity);

    protected:
        virtual void initBuf(PoolChunk* chunk, const char * buffer, long handle,
                             PoolByteBuf* buf, int reqCapacity, PoolThreadCache * threadCache) = 0;
        std::queue<Entry *> m_queue;

    public:
        MemoryRegionCache(int size, SizeClassType sizeClassType);
        virtual ~MemoryRegionCache() {};
        bool add(PoolChunk* chunk, const char * buffer, long handle, int normCapacity);
        bool allocate(PoolByteBuf* buf, int reqCapacity, PoolThreadCache* threadCache);
        //free the thread_local cache, return the freed sum of chunk and handle
        int free();
        // m_queue work in fifo way?? or it is like deque container via push_back and pop_front
        void trim();

    };

    class SubpageMemoryRegionCache final : public MemoryRegionCache {
    public:
        explicit SubpageMemoryRegionCache(int size);

        void initBuf(PoolChunk* chunk, const char * buffer, long handle, PoolByteBuf* buf, int reqCapacity,
                     PoolThreadCache* threadCache) override;

        ~SubpageMemoryRegionCache() override;
    };

    class NormalMemoryRegionCache final : public MemoryRegionCache {
    public:
        explicit NormalMemoryRegionCache(int size);

        void initBuf(PoolChunk* chunk, const char * buffer, long handle, PoolByteBuf* buf, int reqCapacity,
                     PoolThreadCache* threadCache) override;

        ~NormalMemoryRegionCache() override;
    };



}


#endif //LONGADDR_POOLTHREADCACHE_H

