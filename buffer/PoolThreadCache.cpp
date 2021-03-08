//
// Created by gzp on 2021/2/2.
//

#include <limits.h>
#include <iostream>
#include "PoolThreadCache.h"
#include "PoolArena.h"
#include "PoolByteBuf.h"
#include "PoolChunk.h"

namespace buffer {

    PoolThreadCache::PoolThreadCache(PoolArena* arena, int smallCacheSize,
                                     int normalCacheSize, int maxCacheCapaicy, int freeSweepAllocationThreshold)
                                     : m_allocations(0) {
        assert(maxCacheCapaicy >= 0);
        m_freeSweepAllocationThreshold = freeSweepAllocationThreshold;
        m_arena = arena;
        if(m_arena != nullptr) {
            m_smallSubpageCaches = createSubpageCaches(smallCacheSize, m_arena->m_numSmallSubpagePools);
            m_normalCaches = createNormalCaches(normalCacheSize, maxCacheCapaicy, m_arena);
            (m_arena->m_numThreadCaches) ++;
        } else {
            //NOOP
            m_smallSubpageCaches.clear();
            m_normalCaches.clear();
        }

        //  if cached , we need to set m_freeSweepAllocationThreshold to clean out-of-date caches
        if((!m_smallSubpageCaches.empty() || !m_normalCaches.empty()) && m_freeSweepAllocationThreshold < 1)
            throw std::exception();

    }

    PoolThreadCache::~PoolThreadCache() {
        for_each(m_smallSubpageCaches.begin(), m_smallSubpageCaches.end(), DeleteObject());
        for_each(m_normalCaches.begin(), m_normalCaches.end(), DeleteObject());
    }

    std::vector<MemoryRegionCache*> PoolThreadCache::createSubpageCaches(int cacheSize, int numCaches) {
        std::vector<MemoryRegionCache *> cache;
        if(cacheSize > 0 && numCaches > 0) {
            for(int i = 0; i < numCaches; i ++) {
                cache.push_back(new SubpageMemoryRegionCache(cacheSize));
            }
        }
        return cache;
    }

    std::vector<MemoryRegionCache*> PoolThreadCache::createNormalCaches(int cacheSize, int maxCacheCapacity, PoolArena* arena) {
        std::vector<MemoryRegionCache*> cache;
        if(cacheSize > 0 && maxCacheCapacity > 0) {
            int maxCacheSize = std::min(arena->m_chunkSize, maxCacheCapacity);

            for(int i = arena->m_numSmallSubpagePools; i < arena->m_nSizes && arena->sizeIdx2size(i) <= maxCacheSize; i ++)
                cache.push_back(new NormalMemoryRegionCache(cacheSize));
//            return cache;
        }
        return cache;
    }

    bool PoolThreadCache::allocateSmall(PoolArena* arena, PoolByteBuf * buf, int reqCapacity, int sizeIdx) {
        return allocate(cacheForSmall(arena, sizeIdx), buf, reqCapacity);
    }

    bool PoolThreadCache::allocateNormal(PoolArena* arena, PoolByteBuf * buf, int reqCapacity, int sizeIdx) {
        return allocate(cacheForNormal(arena, sizeIdx), buf, reqCapacity);
    }

    //private
    bool PoolThreadCache::allocate(MemoryRegionCache* cache, PoolByteBuf * buf, int reqCapacity) {
        if(cache == nullptr) return false;
        bool allocated = cache->allocate(buf, reqCapacity, this);
        if(++ m_allocations >= m_freeSweepAllocationThreshold) {
            m_allocations = 0;
            trim();
        }
        return allocated;
    }

    bool MemoryRegionCache::allocate(PoolByteBuf* buf, int reqCapacity, PoolThreadCache* threadCache) {
        if(m_queue.empty()) return false;
        Entry * entry = m_queue.front();
        m_queue.pop();
//            if(entry == nullptr) {
//                return false;
//            }
        initBuf(entry->chunk, entry->buffer, entry->handle, buf, reqCapacity, threadCache);
        entry->recycler();
        m_allocations ++;
        return true;
    }

    bool PoolThreadCache::add(PoolArena* arena, PoolChunk* chunk, const char * buffer, long handle,
                              int normCapacity, SizeClassType sizeClassType) {
        int sizeIdx = arena->size2sizeIdx(normCapacity);
        MemoryRegionCache* cache = genCache(arena, sizeIdx, sizeClassType);
        if(cache == nullptr) {
            return false;
        }
        return cache->add(chunk, buffer, handle, normCapacity);
    }

    //private
    MemoryRegionCache* PoolThreadCache::genCache(PoolArena* arena, int sizeIdx, SizeClassType sizeClassType) {
        switch(sizeClassType) {
            case NORMAL:
                return cacheForNormal(arena, sizeIdx);
            case SMALL:
                return cacheForSmall(arena, sizeIdx);
            default:
                throw std::exception(); // ("unknown type for SizeClassType");
        }
    }

    void PoolThreadCache::free() {
        //no need to ulitilize cas
//        int numFreed = free(m_smallSubpageCaches) + free(m_normalCaches);
//        if(numFreed > 0 )
//            std::cout << num
        if(m_arena != nullptr)
            m_arena->m_numThreadCaches --;

    }

    //private
    int PoolThreadCache::free(std::vector<MemoryRegionCache* > & caches) {
        if(caches.empty()) return 0;

        int numFreed = 0;
        for(auto & cache : caches)
            numFreed += free(cache);
        return numFreed;
    }

    //private
    int PoolThreadCache::free(MemoryRegionCache* cache) {
        if(cache == nullptr) return 0;
        return cache->free();
    }

    void PoolThreadCache::trim() {
        trim(m_smallSubpageCaches);
        trim(m_normalCaches);
    }

    void PoolThreadCache::trim(std::vector<MemoryRegionCache*> & caches) {
//        if(caches == nullptr) return;
        if(caches.empty()) return;
        for(auto& cache : caches)
            trim(cache);
    }

    void PoolThreadCache::trim(MemoryRegionCache* cache) {
        if(cache == nullptr) return;
        cache->trim();
    }

    MemoryRegionCache * PoolThreadCache::cacheForSmall(PoolArena* arena, int sizeIdx) {
        return cache(m_smallSubpageCaches, sizeIdx);
    }

    MemoryRegionCache * PoolThreadCache::cacheForNormal(PoolArena * arena, int sizeIdx) {
        int idx = sizeIdx - arena->m_numSmallSubpagePools;
        return cache(m_normalCaches, idx);
    }


    int MemoryRegionCache::free(int freeNum) {
        int numFreed = 0;
        while(!m_queue.empty() && numFreed < freeNum) {
            Entry * entry = m_queue.front();
            m_queue.pop();
            freeEntry(entry);
            numFreed ++;
        }
        return numFreed;
    }

    void MemoryRegionCache::freeEntry(Entry* entry) {
        PoolChunk* chunk = entry->chunk;
        long handle = entry->handle;
        char * buffer = entry->buffer;
        entry->recycler();
        chunk->m_arena->freeChunk(chunk, handle, entry->normCapacity, m_sizeClassType, buffer);
    }

    Recycler<MemoryRegionCache::Entry> * MemoryRegionCache::Entry::m_recycler = new MemoryRegionCache::EntryBufRecycler();

    MemoryRegionCache::Entry * MemoryRegionCache::newEntry(PoolChunk* chunk, const char * buffer, long handle, int normCapacity) {
        Entry* entry = Entry::m_recycler->get();
        entry->chunk = chunk;
        entry->buffer = const_cast<char*>(buffer);
        entry->handle = handle;
        entry->normCapacity = normCapacity;
        return entry;
    }

    MemoryRegionCache::MemoryRegionCache(int size, SizeClassType sizeClassType)
            : m_size(MathUtils::nextPowerofTwo(size)),
              m_sizeClassType(sizeClassType),
              m_allocations(0) {};
//              m_recycler(Recycler<Entry>::getInstance()){};

    bool MemoryRegionCache::add(PoolChunk* chunk, const char * buffer, long handle, int normCapacity) {
        Entry * entry = newEntry(chunk, buffer, handle, normCapacity);
        m_queue.push(entry);
        return true;
    }

    int MemoryRegionCache::free() {
        return free(INT_MAX);
    }

    void MemoryRegionCache::trim() {
        int toFreeNum = m_size - m_allocations;
        m_allocations = 0;
        if(toFreeNum > 0) {
            free(toFreeNum);
        }
    }


    SubpageMemoryRegionCache::SubpageMemoryRegionCache(int size)
    : MemoryRegionCache(size,  SMALL) {};

    void SubpageMemoryRegionCache::initBuf(PoolChunk* chunk, const char * buffer, long handle, PoolByteBuf* buf, int reqCapacity,
                 PoolThreadCache* threadCache) {
        chunk->initBufWithSubpage(buf, buffer, handle, reqCapacity, threadCache);
    }

    SubpageMemoryRegionCache::~SubpageMemoryRegionCache() {
        while(!m_queue.empty()) {
            if(m_queue.front() != nullptr)  {
                delete m_queue.front();
                m_queue.pop();
            }
        }
    }


    NormalMemoryRegionCache::NormalMemoryRegionCache(int size)
    : MemoryRegionCache(size,  NORMAL) {};

    void NormalMemoryRegionCache::initBuf(PoolChunk* chunk, const char * buffer, long handle, PoolByteBuf* buf, int reqCapacity,
                 PoolThreadCache* threadCache) {
        chunk->initBuf(buf, buffer, handle, reqCapacity, threadCache);
    }

    NormalMemoryRegionCache::~NormalMemoryRegionCache() {
        while(!m_queue.empty()) {
            if(m_queue.front() != nullptr)  {
                delete m_queue.front();
                m_queue.pop();
            }
        }
    }
}