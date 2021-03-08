//
// Created by gzp on 2021/2/11.
//


#include "PoolByteBufAllocator.h"
#include "PoolArena.h"
#include "PoolThreadCache.h"
#include "PoolChunk.h"

namespace buffer {

    PoolByteBufAllocator::PoolByteBufAllocator()
    {
        poolByteBufAllocatorInit(DEFAULT_NUM_ARENA, DEFAULT_PAGE_SIZE, DEFAULT_MAX_ORDER,
                                 DEFAULT_SMALL_CACHE_SIZE, DEFAULT_NORMAL_CACHE_SIZE);
    }

    PoolByteBufAllocator * PoolByteBufAllocator::DEFAULT_ALLOC = nullptr;

    int PoolByteBufAllocator::m_smallCacheSize = 0;
    int PoolByteBufAllocator::m_normalCahceSize = 0;
    int PoolByteBufAllocator::m_chunkSize = 0;
    std::vector<PoolArena *> PoolByteBufAllocator::m_arenas;
    std::mutex PoolByteBufAllocator::m_lockSingleton;
    SpinLock PoolByteBufAllocator::m_lockThreadcache;

    thread_local PoolThreadCache * PoolByteBufAllocator::m_poolTheadCache(nullptr);
    int PoolByteBufAllocator::DEFAULT_NUM_ARENA = base::Dependence::processNum();

    PoolByteBufAllocator * PoolByteBufAllocator::ALLOCATOR() {
        if(DEFAULT_ALLOC == nullptr) {
            std::unique_lock<std::mutex> lock(m_lockSingleton, std::defer_lock);
            lock.lock();
            if(DEFAULT_ALLOC == nullptr) {
                DEFAULT_ALLOC = new PoolByteBufAllocator();
            }
            lock.unlock();
        }
        if(m_poolTheadCache == nullptr)
            m_poolTheadCache = initThreadLocalCache();
//        std::cout << "m_poolthreadcache address: "<<m_poolTheadCache<<" arena address: "<<m_poolTheadCache->m_arena<<std::endl;
        return DEFAULT_ALLOC;
    }

    PoolThreadCache * PoolByteBufAllocator::initThreadLocalCache() {
        PoolArena * leastArena = leastUsedArena(m_arenas);
        //std::cout << thread <<" was assigned to " <<leastArena
        PoolThreadCache * cache = new PoolThreadCache(leastArena, m_smallCacheSize,m_normalCahceSize,
                                                      DEFAULT_MAX_CACHED_BUFFER_CAPACITY, DEFAULT_CACHE_TRIM_INTERVAL);
        return cache;
    }

    PoolArena * PoolByteBufAllocator::leastUsedArena(std::vector<PoolArena *>& arenas) {
        PoolArena *minArena = m_arenas[0];
        if(arenas.empty()) return nullptr;
        {
            std::unique_lock<SpinLock> lock(m_lockThreadcache);
            for (int i = 0; i < m_arenas.size(); i++) {
                PoolArena *arena = m_arenas[i];
                if (arena->m_numThreadCaches.load() < minArena->m_numThreadCaches.load()) {
                    minArena = arena;
                }
            }
        }
        return minArena;
    }

    void PoolByteBufAllocator::poolByteBufAllocatorInit(int nArena, int pageSize, int maxOrder,
                                                        int smallCacheSize, int normalCacheSize) {
//        assert(nArena > 0);
//        m_poolTheadCache = new PoolThreadCache();
        m_smallCacheSize = smallCacheSize;
        m_normalCahceSize = normalCacheSize;
        m_chunkSize = validateAndCalculateChunkSize(pageSize, maxOrder);
        int pageShifts = validateAndCalculatePageShifts(pageSize);

        if (nArena > 0) {
            m_arenas = newArenaArray(nArena);
//            List<PoolArenaMetric> metrics = new ArrayList<PoolArenaMetric>(directArenas.length);
            for (int i = 0; i < m_arenas.size(); i ++) {
                DefaultArena * arena = new DefaultArena(
                        this, pageSize, pageShifts, m_chunkSize);
                m_arenas[i] = dynamic_cast<PoolArena *>(arena);
//                metrics.add(arena);
            }
//            directArenaMetrics = Collections.unmodifiableList(metrics);
        }
    }

    std::vector<PoolArena * > PoolByteBufAllocator::newArenaArray(int size) {
        std::vector<PoolArena * > poolArenaArray(size, nullptr);
        return poolArenaArray;
    }

    int PoolByteBufAllocator::validateAndCalculatePageShifts(int pageSize) {
        if(pageSize < MIN_PAGE_SIZE) {
            std::string s("pagesize is too small");
            throw std::exception();
        }
        if((pageSize & pageSize - 1) != 0) {
//            throw std::exception("pagesize needs to be power of 2");
            throw std::exception();
        }
        return MathUtils::log2(pageSize);
    }

    int PoolByteBufAllocator::validateAndCalculateChunkSize(int pageSize, int maxOrder) {
        assert(maxOrder < 14);
        int chunkSize = pageSize;
        for(int i = maxOrder; i > 0; i--) {
            if(chunkSize > MAX_CHUNK_SIZE / 2) {
//                throw exception("pagesize << maxorder must not overflow");
                throw std::exception();
            }
            chunkSize <<= 1;
        }
        return chunkSize;
    }

    PoolByteBuf * PoolByteBufAllocator::poolBuffer() {
        return poolBuffer(DEFAULT_INITIAL_CAPACITY, DEFAULT_MAX_CAPACITY);
    }

    PoolByteBuf * PoolByteBufAllocator::poolBuffer(int initCapacity) {
        return poolBuffer(initCapacity, DEFAULT_MAX_CAPACITY);
    }

    PoolByteBuf * PoolByteBufAllocator::poolBuffer(int initCapacity, int maxCapacity) {
        if(initCapacity == 0 && maxCapacity == 0)
//            return emptyBuf;
            return nullptr;
//        validate(initCapacity, maxCapacity);
        assert(initCapacity >= 0 && initCapacity <= maxCapacity);
        return newPoolBuffer(initCapacity, maxCapacity);
    }


    void PoolByteBufAllocator::validate(int initialCapacity, int maxCapacity) {
        assert(initialCapacity >= 0);
        if (initialCapacity > maxCapacity) {
//            throw exception("Initial capacity must not be greater than maxCapacity");
            throw std::exception();
        }
    }

    PoolByteBuf * PoolByteBufAllocator::newPoolBuffer(int initCapacity, int maxCapacity) {
        PoolArena * defaultArena = m_poolTheadCache->m_arena;
        PoolByteBuf * buf = nullptr;
        if(defaultArena != nullptr) {
            buf = defaultArena->allocate(m_poolTheadCache, initCapacity, maxCapacity);
        } else {
            //buf =
//            throw exception("Try to allocate PoolByteBuf while having no PoolArena");
            throw std::exception();
        }
        return buf;
    }

    PoolThreadCache * PoolByteBufAllocator::threadCache() {
        assert(m_poolTheadCache != nullptr);
        return m_poolTheadCache;
    }

    int PoolByteBufAllocator::calculateNewCapacity(int minNewCapacity, int maxCapacity) {
        assert(minNewCapacity >= 0);
        if(minNewCapacity > maxCapacity) {
            throw std::exception();
        }
        const int threshold = CALCULATE_THRESHOLD;
        if(minNewCapacity == threshold) {
            return threshold;
        }
        if(minNewCapacity > threshold) {
            int newCapacity = minNewCapacity / threshold * threshold;
            if(newCapacity > maxCapacity - threshold) {
                newCapacity = maxCapacity;
            } else {
                newCapacity += threshold;
            }
            return newCapacity;
        }
        int newCapacity = 64;
        while(newCapacity < minNewCapacity) {
            newCapacity <<= 1;
        }
        return std::min(newCapacity, maxCapacity);
    }

}

