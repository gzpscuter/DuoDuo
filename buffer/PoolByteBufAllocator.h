//
// Created by gzp on 2021/2/11.
//

#ifndef LONGADDR_POOLBYTEBUFALLOCATOR_H
#define LONGADDR_POOLBYTEBUFALLOCATOR_H

#include <limits.h>
#include <exception>
#include <vector>
#include <mutex>
#include "commons.h"
#include "../base/Dependence.h"
#include "LongAddr/SpinLock.h"


namespace buffer {
    class PoolThreadCache;
    class PoolByteBuf;
    class DefaultArena;
    class PoolChunk;
    class PoolArena;

    class PoolByteBufAllocator {

        void poolByteBufAllocatorInit(int nArena, int pageSize, int maxOrder, int smallCacheSize, int normalCacheSize);
        static std::vector<PoolArena * > newArenaArray(int size);
        static int validateAndCalculatePageShifts(int pageSize);
        static int validateAndCalculateChunkSize(int pageSize, int maxOrder);
        void validate(int initialCapacity, int maxCapacity);
        PoolByteBuf * newPoolBuffer(int initCapacity, int maxCapacity);
        static PoolArena * leastUsedArena(std::vector<PoolArena *>& arenas);
        PoolByteBufAllocator();

        static thread_local PoolThreadCache * m_poolTheadCache;
        static std::vector<PoolArena *> m_arenas;
        static int m_smallCacheSize;
        static int m_normalCahceSize;
        static int m_chunkSize;


        static PoolByteBufAllocator * DEFAULT_ALLOC;
        static std::mutex m_lockSingleton;
        static SpinLock m_lockThreadcache;
    public:

        static int DEFAULT_NUM_ARENA;
        static const int DEFAULT_PAGE_SIZE = 8192;
        static const int DEFAULT_MAX_ORDER = 11;
        static const int DEFAULT_SMALL_CACHE_SIZE = 256;
        static const int DEFAULT_NORMAL_CACHE_SIZE = 64;
        static const int DEFAULT_MAX_CACHED_BUFFER_CAPACITY = 32 * 1024;
        static const int DEFAULT_CACHE_TRIM_INTERVAL = 8192;
//    static int DEFAULT_CACHE_TRIM_INTERVAL_MILLIS
        static const int DEFAULT_MAX_CACHED_BYTES_PER_CHUNK = 1023;
        static const int MIN_PAGE_SIZE = 4096;
        static const int MAX_CHUNK_SIZE = (int) (((long) INT_MAX + 1) / 2);
        static const int DEFAULT_INITIAL_CAPACITY = 256;
        static const int DEFAULT_MAX_CAPACITY = INT_MAX;
        static const int DEFAULT_MAX_COMPONENTS = 16;
        static const int CALCULATE_THRESHOLD = 1048576 * 4;

//        static PoolByteBuf *m_poolByteBuf = new PoolByteBuf();
        PoolByteBufAllocator(const PoolByteBufAllocator&) = delete;

        PoolByteBuf * poolBuffer();
        PoolByteBuf * poolBuffer(int initCapacity);
        PoolByteBuf * poolBuffer(int initCapacity, int maxCapacity);
        // ALLOCATOR是PoolByteBufAllocator类的静态函数，使用单例模式，用于线程获取一个PoolByteBufAllocator对象
        // 以及初始化thread_local类型的m_poolThreadCache
        static PoolByteBufAllocator * ALLOCATOR();

        static PoolThreadCache * initThreadLocalCache();
        PoolThreadCache * threadCache();
        int calculateNewCapacity(int minNewCapacity, int maxCapacity);

    };

}
#endif //LONGADDR_POOLBYTEBUFALLOCATOR_H
