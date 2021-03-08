//
// Created by gzp on 2021/2/4.
//

#ifndef LONGADDR_POOLCHUNKLIST_H
#define LONGADDR_POOLCHUNKLIST_H

#include <numeric>
#include <vector>
#include "commons.h"


namespace buffer {
    class PoolArena;
    class PoolChunk;
    class PoolByteBuf;
    class PoolThreadCache;

    class PoolChunkList final {

        int calculateMaxCapacity(int minUsage, int chunkSize);
        bool move(PoolChunk* chunk);
        bool move0(PoolChunk* chunk);
        void remove(PoolChunk* cur);

        PoolArena* m_arena;
        PoolChunkList* m_prevList;
        PoolChunkList* m_nextList;
        int m_minUsage;
        int m_maxUsage;
        int m_maxCapacity;
        PoolChunk* m_head;
        int m_freeMinThreshold;
        int m_freeMaxThreshold;

    public:

        PoolChunkList(PoolArena* arena, PoolChunkList * nextList, int minUsage, int maxUsage, int chunkSize);
        void prevList(PoolChunkList* prevList);
        bool allocate(PoolByteBuf * buf, int reqCapacity, int sizeIdx, PoolThreadCache * threadCache);
        bool free(PoolChunk* chunk, long handle, int normCapacity, const char * buffer);
        void add(PoolChunk* chunk);
        void add0(PoolChunk* chunk);
        int minUsage();
        int maxUsage();

    };

}


#endif //LONGADDR_POOLCHUNKLIST_H
