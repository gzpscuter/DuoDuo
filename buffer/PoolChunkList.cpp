//
// Created by gzp on 2021/2/4.
//

#include "PoolChunkList.h"
#include "PoolArena.h"
#include "PoolChunk.h"
#include "PoolByteBuf.h"
#include "PoolThreadCache.h"

namespace buffer {
    PoolChunkList::PoolChunkList(PoolArena* arena, PoolChunkList * nextList, int minUsage, int maxUsage, int chunkSize)
    : m_prevList(nullptr), m_head(nullptr) {
        assert(arena != nullptr && minUsage <= maxUsage);
        m_arena = arena;
        m_nextList = nextList;
        m_minUsage = minUsage;
        m_maxUsage = maxUsage;
        m_maxCapacity = calculateMaxCapacity(minUsage, chunkSize);

        m_freeMinThreshold = (maxUsage == 100) ? 0 : static_cast<int>(chunkSize * (100.0 - m_maxUsage + 0.99999999) / 100L);
        m_freeMaxThreshold = (minUsage == 100) ? 0 : static_cast<int>(chunkSize * (100.0 - m_minUsage + 0.99999999) / 100L);
    }

    int PoolChunkList::calculateMaxCapacity(int minUsage, int chunkSize) {
        m_minUsage = std::max(0, minUsage);
        if(m_minUsage == 100) return 0;
        return static_cast<int>(chunkSize * (100L - m_minUsage) / 100L);
    }

    void PoolChunkList::prevList(PoolChunkList* prevList) {
        assert(m_prevList == nullptr);
        m_prevList = prevList;
    }

    bool PoolChunkList::allocate(PoolByteBuf * buf, int reqCapacity, int sizeIdx, PoolThreadCache * threadCache) {
        int normCapacity = m_arena->sizeIdx2size(sizeIdx);
        if(normCapacity > m_maxCapacity)
            return false;
        for(PoolChunk* cur = m_head; cur != nullptr; cur = cur->next) {
            if(cur->allocate(buf, reqCapacity, sizeIdx, threadCache)) {
                if(cur->m_freeBytes <= m_freeMinThreshold) {
                    remove(cur);
                    m_nextList->add(cur);
                }
                return true;
            }
        }
        return false;
    }

    bool PoolChunkList::free(PoolChunk* chunk, long handle, int normCapacity, const char * buffer) {
        chunk->free(handle, normCapacity, buffer);
        if(chunk->m_freeBytes > m_freeMaxThreshold) {
            remove(chunk);
            return move0(chunk);
        }
        return true;
    }

    // move : from high-used  to low-used
    // add : from low-used to high-used
    bool PoolChunkList::move(PoolChunk* chunk) {
        assert(chunk->usage() < m_maxUsage);

        if(chunk->m_freeBytes > m_freeMaxThreshold)
            return move0(chunk);
        add0(chunk);
        return true;
    }

    //private
    bool PoolChunkList::move0(PoolChunk* chunk) {
        if(m_prevList == nullptr) {
            assert(chunk->usage() == 0);
            return false;
        }
        return m_prevList->move(chunk);
    }

    // add was called when the memory regions are allocated
    void PoolChunkList::add(PoolChunk* chunk) {
        if(chunk->freeBytes() <= m_freeMinThreshold) {
            m_nextList->add(chunk);
            return;
        }
        add0(chunk);
        return;
    }

    //add PoolChunk to the first of PoolChunkList, on the reason that the more lately the released memory is the more likely its cache in cpu
    void PoolChunkList::add0(PoolChunk* chunk) {
        chunk->m_parent = this;
        if(m_head == nullptr) {
            m_head = chunk;
            chunk->prev = nullptr;
            chunk->next = nullptr;
        } else {
            chunk->prev = nullptr;
            chunk->next = m_head;
            m_head->prev = chunk;
            m_head = chunk;
        }
    }

    //private
    // remove is used to relase the target chunk, set its next and prev ones to nullptr, such that the chunk unbinds from chunkList
    void PoolChunkList::remove(PoolChunk* cur) {
        if(cur == m_head) {
            m_head = m_head->next;
            if(m_head != nullptr)
                m_head->prev = nullptr;
        } else {
            auto next = cur->next;
            cur->prev->next = next;
            if(next != nullptr)
                next->prev = cur->prev;
        }
    }

    int PoolChunkList::minUsage() {
        return std::max(1, m_minUsage);
    }

    int PoolChunkList::maxUsage() {
        return std::min(m_maxUsage, 100);
    }

}