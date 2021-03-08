//
// Created by gzp on 2021/1/26.
//

#include <iostream>
#include <limits.h>
#include "PoolArena.h"
#include "PoolThreadCache.h"
#include "PoolByteBuf.h"
#include "PoolChunkList.h"
#include "PoolSubpage.h"
#include "PoolByteBufAllocator.h"
#include "PoolChunk.h"

namespace buffer {

    PoolArena::PoolArena(PoolByteBufAllocator * parent, int pageSize,
                            int pageShifts, int chunkSize)
    :SizeClass(pageSize, pageShifts, chunkSize),
    m_deallocationSmall(0),
    m_deallocationNormal(0),
    m_numThreadCaches(0),
    m_allocationsNormal(0)
    {
        m_parent = parent;
        m_numSmallSubpagePools = m_nSubpages;
        m_smallSubpagePools.assign(m_numSmallSubpagePools, nullptr);

        for(int i = 0; i < m_smallSubpagePools.size(); i++) {
            m_smallSubpagePools[i] = newSubpagePoolHead();
        }

        q100 = new PoolChunkList(this, nullptr, 100, INT_MAX, chunkSize);
        q75 = new PoolChunkList(this, q100, 75, 100, chunkSize);
        q50 = new PoolChunkList(this, q75, 50, 100, chunkSize);
        q25 = new PoolChunkList(this, q50, 25, 75, chunkSize);
        q00 = new PoolChunkList(this, q25, 1, 50, chunkSize);
        qInit = new PoolChunkList(this, q00, INT_MIN, 25, chunkSize);

        q100->prevList(q75);
        q75->prevList(q50);
        q50->prevList(q25);
        q25->prevList(q00);
        q00->prevList(nullptr);
        qInit->prevList(qInit);
    }

    //private
    PoolSubpage * PoolArena::newSubpagePoolHead() {
        PoolSubpage* head = new PoolSubpage();
        head->next = head;
        head->prev = head;
        return head;
    }

    PoolByteBuf * PoolArena::allocate(PoolThreadCache* cache, int reqCapacity, int maxCapacity) {
        PoolByteBuf * buf = newByteBuf(maxCapacity);
        allocate(cache, buf, reqCapacity);
        return buf;

    }

    //private
    // this func is to allocate memory out of the PoolArena and its components based on required-capacity
    void PoolArena::allocate(PoolThreadCache* cache, PoolByteBuf* buf, int reqCapacity) {
        int sizeIdx = size2sizeIdx(reqCapacity);
        if(sizeIdx <= m_smallMaxSizeIdx) {
//            std::cout << "m_samllMaxSizeIdx branch " << reqCapacity << std::endl;
            tcacheAllocateSmall(cache, buf, reqCapacity, sizeIdx);
        } else if(sizeIdx < m_nSizes) {
//            std::cout << "m_nSizes branch " << reqCapacity << std::endl;
            tcacheAllocateNormal(cache, buf, reqCapacity, sizeIdx);
        } else {
            int normCapacity = normalizeSize(reqCapacity);
            allocateHuge(buf, normCapacity);
        }
    }

    //private
    void PoolArena::tcacheAllocateSmall(PoolThreadCache* cache, PoolByteBuf * buf, const int reqCapacity, const int sizeIdx) {
        if(cache->allocateSmall(this, buf, reqCapacity, sizeIdx)) return ;

        PoolSubpage * head = m_smallSubpagePools[sizeIdx];
        bool needNormAllocation = false;

        std::unique_lock<std::mutex> lockSubpage(head->m_mtx, std::defer_lock);
        lockSubpage.lock();
        PoolSubpage * s = head->next;
        needNormAllocation = s == head;
        if(!needNormAllocation) {
            assert(s->m_dontRelase && s->m_elemSize == sizeIdx2size(sizeIdx));
            long handle = s->allocate();
            assert(handle >= 0);
            s->m_chunk->initBufWithSubpage(buf, nullptr, handle, reqCapacity, cache);
        }
        lockSubpage.unlock();

        if(needNormAllocation) {
            std::unique_lock<std::mutex> lockPages(m_mtx);
            allocateNormal(buf, reqCapacity, sizeIdx, cache);
            lockPages.unlock();
        }

        m_allocationSmall.add(1L);

    }

    // private
    void PoolArena::tcacheAllocateNormal(PoolThreadCache* cache, PoolByteBuf * buf, const int reqCapacity, const int sizeIdx) {
        if(cache->allocateNormal(this, buf, reqCapacity, sizeIdx)) return;
        std::unique_lock<std::mutex> lock(m_mtx);
        allocateNormal(buf, reqCapacity, sizeIdx, cache);
        m_allocationsNormal ++;
    }

    //private
    void PoolArena::allocateNormal(PoolByteBuf * buf, int reqCapacity, int sizeIdx, PoolThreadCache* threadCache) {
        if( q50->allocate(buf, reqCapacity, sizeIdx, threadCache) ||
            q25->allocate(buf, reqCapacity, sizeIdx, threadCache) ||
            q00->allocate(buf, reqCapacity, sizeIdx, threadCache) ||
            qInit->allocate(buf, reqCapacity, sizeIdx, threadCache) ||
            q75->allocate(buf, reqCapacity, sizeIdx, threadCache))
            return;

        PoolChunk* chunk = newChunk(m_pageSize, m_nPSizes, m_pageShifts, m_chunkSize);
        bool ret = chunk->allocate(buf, reqCapacity, sizeIdx, threadCache);
        assert(ret);
        qInit->add(chunk);

    }

    //private
    void PoolArena::allocateHuge(PoolByteBuf * buf, int reqCapacity) {
        PoolChunk* chunk = newUnpoolChunk(reqCapacity);
        m_activeBytesHuge.add(chunk->chunkSize());
        buf->initUnpool(chunk, reqCapacity);
        m_allocationHuge.add(1L);
    }

    void PoolArena::free(PoolChunk* chunk, const char * buffer, long handle, int normCapacity, PoolThreadCache* cache) {
        if(chunk->m_unpool) {
            int size = chunk->chunkSize();
            destroyChunk(chunk);
            m_activeBytesHuge.add(-size);
            m_deallocationHuge.add(1L);
        } else {
            SizeClassType sizeClassType = getSizeClassType(handle);
            if(cache != nullptr && cache->add(this, chunk, buffer, handle, normCapacity, sizeClassType))
                return;
            freeChunk(chunk, handle, normCapacity, sizeClassType, buffer);
        }
    }

    enum SizeClassType PoolArena::getSizeClassType(long handle) {
        return PoolChunk::isSubpage(handle) ? SMALL : NORMAL;
    }

    void PoolArena::freeChunk(PoolChunk* chunk, long handle, int normCapacity, SizeClassType sizeClassType, const char * buffer) {
        bool destroyChunkRes;
        std::unique_lock<std::mutex> lock(m_mtx, std::defer_lock);
        lock.lock();

        switch (sizeClassType) {
            case NORMAL :
                m_deallocationNormal++;
                break;
            case SMALL :
                m_deallocationSmall ++;
                break;
            default:
                throw std::exception();
        }
        destroyChunkRes = !chunk->m_parent->free(chunk, handle, normCapacity, buffer);

        lock.unlock();

        if (destroyChunkRes) {
            // destroyChunk not need to be called while holding the synchronized lock.
            // 当free后chunk到达了最前面的qinit，也即m_prevList==nullptr，则说明chunk未使用，可以释放
            destroyChunk(chunk);
        }
    }

    PoolSubpage* PoolArena::findSubpagePoolHead(int sizeIdx) {
        return m_smallSubpagePools[sizeIdx];
    }

    void PoolArena::reallocate(PoolByteBuf * buf, int newCapacity, bool freeOldMemory) {
        assert( newCapacity >= 0 && newCapacity <= buf->maxCapacity() );
        int oldCapacity = buf->length();
        if(oldCapacity == newCapacity) return;

        PoolChunk* oldChunk = buf->chunk();
        char * oldBuffer = buf->tmpBuf();
        long oldHandle = buf->handle();
        char * oldMemory = buf->memory();
        int oldOffset = buf->offset();
        int oldMaxLength = buf->maxLength();

        allocate(m_parent->threadCache(), buf, newCapacity);
        int bytesTocoy;
        if(newCapacity > oldCapacity)
            bytesTocoy = oldCapacity;
        else {
            buf->trimIndicesToCapacity(newCapacity);
            bytesTocoy = newCapacity;
        }
        memoryCopy(oldMemory, oldOffset, buf, bytesTocoy);
        if(freeOldMemory)
            free(oldChunk, oldBuffer, oldHandle, oldMaxLength, buf->cache());

    }


    DefaultArena::DefaultArena(PoolByteBufAllocator * parent,
                               int pageSize, int pageShifts,int chunkSize)
                               : PoolArena(parent, pageSize, pageShifts, chunkSize){

    }

    //protected
    PoolChunk * DefaultArena::newChunk(int pageSize, int maxPageIdx, int pageShifts, int chunkSize) {
        return new PoolChunk(this, defaultAllocate(chunkSize), pageSize, pageShifts, chunkSize, maxPageIdx, 0);
    }


    PoolChunk * DefaultArena::newUnpoolChunk(int capacity) {
        return new PoolChunk(this, defaultAllocate(capacity), capacity, 0);
    }


    char * DefaultArena::defaultAllocate(int capacity) {
        return new char[capacity];
    }


    void DefaultArena::destroyChunk(PoolChunk * chunk) {
        delete chunk;
    }


    PoolByteBuf * DefaultArena::newByteBuf(int maxCapacity) {
        return PoolByteBuf::newInstance(maxCapacity);
    }

    void DefaultArena::memoryCopy(const char* memory, int srcOffset, PoolByteBuf * dstBuf, int length) {
        if(length == 0) return;
        std::copy(memory + srcOffset, memory + srcOffset + length, dstBuf->memory() + dstBuf->offset());
    }

}
