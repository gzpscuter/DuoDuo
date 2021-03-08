//
// Created by gzp on 2021/1/25.
//

#include <iostream>
#include "PoolChunk.h"
#include "PoolThreadCache.h"
#include "PoolArena.h"
#include "PoolChunkList.h"
#include "PoolSubpage.h"
#include "PoolByteBuf.h"
#include "PoolByteBufAllocator.h"

namespace buffer {


    PoolChunk::PoolChunk(PoolArena *arena, const char *memory, int pageSize, int pageShifts, int chunkSize,
                         int maxPageIdx, int offset)
    : m_arena(arena),
      m_memory(const_cast<char*>(memory)),
      m_pageSize(pageSize),
      m_pageShifts(pageShifts),
      m_chunkSize(chunkSize),
      m_offset(offset),
      m_unpool(false),
      prev(nullptr),
      next(nullptr),
      m_parent(nullptr) {

        m_freeBytes = chunkSize;
        m_runsAvail = std::move(newRunsAvailQueue(maxPageIdx));
        int pages = m_chunkSize >> m_pageShifts;
        m_subpages = std::move(newPoolSubpageArray(pages));
        long initHandler = static_cast<long>(pages) << SIZE_SHIFT_;

        insertAvailRun(0, pages, initHandler);
    }

    PoolChunk::PoolChunk(PoolArena * arena, const char * memory, int size, int offset)
    : m_unpool(true),
    m_arena(arena),
    m_memory(const_cast<char*>(memory)),
    m_offset(offset),
    m_pageSize(0),
    m_pageShifts(0),
    m_chunkSize(size),
    /*m_subpages(nullptr), */m_freeBytes(0), m_parent(nullptr), prev(nullptr), next(nullptr)
    {
    }

    PoolChunk::~PoolChunk() {
        if(nullptr != m_memory) {
            delete[] m_memory;
            m_memory = nullptr;
        }

        for_each(m_runsAvail.begin(), m_runsAvail.end(), DeleteObject());
        for_each(m_subpages.begin(), m_subpages.end(), DeleteObject());

    }

    std::vector<BestFitQueue<long>*> PoolChunk::newRunsAvailQueue(int size) {
//        longPriorityQueue * queueArray = new longPriorityQueue[size];
//        m_runsAvail.assign(size, new BestFitQueue<long>());
        std::vector<BestFitQueue<long>*> runsAvail;
        assert(size >= 0);
        for(int i = 0; i < size; i++) {
            runsAvail.push_back(new BestFitQueue<long>());
        }
        return runsAvail;
    }

    std::vector<PoolSubpage*> PoolChunk::newPoolSubpageArray(int size) {
        std::vector<PoolSubpage*> subpages;
        assert(size >= 0);
        for(int i = 0; i < size; i++) {
            subpages.emplace_back(nullptr);
        }
        return subpages;
    }

    void PoolChunk::insertAvailRun(int runOffset, int pages, long handle) {
        int pageIdxFloor = m_arena->page2pageIdxFloor(pages);
//        std::cout <<"insertAvailRUn func: "<<handle<<" pageIdxLift "<<pageIdxLift <<" pages "<<pages<<std::endl;
        BestFitQueue<long>* sinQueue = m_runsAvail[pageIdxFloor];
        sinQueue->push(handle);

        insertAvailRun0(runOffset, handle);
        if (pages > 1)
            insertAvailRun0(tailPage(runOffset, pages), handle);
    }

    int PoolChunk::tailPage(int runOffset, int pages) {
        return runOffset + pages - 1;
    }

    void PoolChunk::insertAvailRun0(int runOffset, long handle) {
        m_runsAvailMap.insert(std::make_pair(runOffset, handle));
    }

    bool PoolChunk::allocate(PoolByteBuf * buf, int reqCapacity, int sizeIdx, PoolThreadCache * cache) {
        long handle = 0L;
        if (sizeIdx <= m_arena->m_smallMaxSizeIdx) {
            // small region
            handle = allocateSubpage(sizeIdx);
            if (handle < 0) return false;
            assert(isSubpage(handle));
        } else {
            // normal region
            int runSize = m_arena->sizeIdx2size(sizeIdx);
            handle = allocateRun(runSize);
            if (handle < 0) return false;
        }
        char *buffer = m_cachedBuffers.empty() ? nullptr : m_cachedBuffers.back();
        if(!m_cachedBuffers.empty())
            m_cachedBuffers.pop_back();
        initBuf(buf, buffer, handle, reqCapacity, cache);
        return true;
    }

    void PoolChunk::removeAvailRun(long handle) {
        int pageIdxFloor = m_arena->page2pageIdxFloor(calculateRunPages(handle));
        BestFitQueue<long>* queue = m_runsAvail[pageIdxFloor];
        removeAvailRun(queue, handle);
    }

    void PoolChunk::removeAvailRun(BestFitQueue<long>* bestFitQueue, long handle) {
        bestFitQueue->remove(handle);

        int runOffset = calculateRunOffset(handle);
        int pages = calculateRunPages(handle);
        //remove first page of run
        m_runsAvailMap.erase(runOffset);
        if (pages > 1) {
            //remove last page of run
            m_runsAvailMap.erase(tailPage(runOffset, pages));
        }
    }

    int PoolChunk::usage() {
        //lock arena?
//        return usage(m_freeBytes);
        if(m_freeBytes == 0) return 100;
        int freeBytePercentage = static_cast<int>(m_freeBytes * 100L / m_chunkSize);
        if(freeBytePercentage == 0) return 99;
        return 100 - freeBytePercentage;
    }

//    int PoolChunk::usage(int freeBytes) {
//        if(freeBytes == 0) return 100;
//        int freeBytePercentage = static_cast<int>(freeBytes * 100L / m_chunkSize);
//        if(freeBytePercentage == 0) return 99;
//        return 100 - freeBytePercentage;
//    }

    long PoolChunk::allocateSubpage(int sizeIdx) {
        PoolSubpage * head = m_arena->findSubpagePoolHead(sizeIdx);
        std::unique_lock<std::mutex> lock(m_subpageAllocMtx);

        int runSize = calculateRunSize(sizeIdx);
        long runHandle = allocateRun(runSize);
        if(runHandle < 0) return -1;

        int runOffset = calculateRunOffset(runHandle);
        assert(m_subpages[runOffset] == nullptr);
        int elemSize = m_arena->sizeIdx2size(sizeIdx);

        PoolSubpage* subpage = new PoolSubpage(head, this, m_pageShifts, runOffset,
                                               calculateRunSize(m_pageShifts, runHandle), elemSize);

        m_subpages[runOffset] = subpage;
        return subpage->allocate();

    }

    long PoolChunk::allocateRun(int runSize) {
        int pages = runSize >> m_pageShifts;
        int pageIdx = m_arena->page2pageIdx(pages);
        std::unique_lock<std::mutex> lock(m_runAvailMtx);

        int runAvailIdx = runBestFit(pageIdx);
        if(runAvailIdx == -1) return -1;
//        std::cout << "runAvailIdx : "<<runAvailIdx << " pageIdx "<<pageIdx << " runsize "<<runSize<< std::endl;
        BestFitQueue<long>* bestFitQueue = m_runsAvail[runAvailIdx];
        //allocate the newest freed region for the reason that it's most likely to be cached in cpu-cache
        assert(!bestFitQueue->empty());
        long handle = bestFitQueue->top();
        assert(!isUsed(handle));
        bestFitQueue->pop();

        removeAvailRun(bestFitQueue, handle);
        if(handle != -1)
            handle = splitLargeRun(handle, pages);
        m_freeBytes -= calculateRunSize(m_pageShifts, handle);
        return handle;
    }

    int PoolChunk::runSize(int pageShifts, long handle) {
        return calculateRunPages(handle) << pageShifts;
    }

    int PoolChunk::calculateRunSize(int sizeIdx) {
        int maxElements = 1 << m_pageShifts - SizeClass::LOG2_QUANTUM;
        int runSize = 0;
        int nElements = 0;
        const int elemSize = m_arena->sizeIdx2size(sizeIdx);
        do {
            runSize += m_pageSize;
            nElements = runSize / elemSize;
        } while(nElements < maxElements && runSize != nElements * elemSize);

        while(nElements > maxElements) {
            runSize -= m_pageSize;
            nElements = runSize / elemSize;
        }
        assert (nElements > 0);
        assert (runSize <= m_chunkSize);
        assert (runSize >= elemSize);

        return runSize;
    }

    int PoolChunk::runBestFit(int pageIdx) {
        if(m_freeBytes == m_chunkSize) {
            return m_arena->m_nPSizes - 1;
        }
        for(int i = pageIdx; i < m_arena->m_nPSizes; i ++) {
            BestFitQueue<long>* queue = m_runsAvail[i];
            if(!queue->empty())
                return i;
        }
        return -1;
    }

    long PoolChunk::splitLargeRun(long handle, int needPages) {
        assert(needPages > 0);

        int totalPages = calculateRunPages(handle);
        assert(needPages <= totalPages);

        int remainPages = totalPages - needPages;
        if(remainPages > 0) {
            int runOffset = calculateRunOffset(handle);
            int availOffset = runOffset + needPages;
            long availRun = toRunHandle(availOffset, remainPages, 0);
//            std::cout << "splitRun "<<"availOffset "<<availOffset<<" remainPages "<<remainPages<<" runOffset "<<runOffset
//            <<" availRUn "<<availRun << std::endl;
            insertAvailRun(availOffset, remainPages, availRun);

            return toRunHandle(runOffset, needPages, 1);
        }
        // mark the run as used
        handle |= 1L << IS_INUSED_SHIFT_;
        return handle;
    }

    long PoolChunk::toRunHandle(int runOffset, int runPages, int inUsed) {
        return static_cast<long>(runOffset) << RUN_OFFSET_SHIFT_
                | static_cast<long>(runPages) << SIZE_SHIFT_
                | static_cast<long>(inUsed) << IS_INUSED_SHIFT_;
    }

    void PoolChunk::free(long handle, int normCapacity, const char * buffer) {
        if(isSubpage(handle)) {
            // free subpage
            int sizeIdx = m_arena->size2sizeIdx(normCapacity);
            PoolSubpage * head = m_arena->findSubpagePoolHead(sizeIdx);

            int sIdx = calculateRunOffset(handle);
            PoolSubpage* subpage = m_subpages[sIdx];
            assert(subpage != nullptr && subpage->m_dontRelase);

            // need lock !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! in PoolSubpage class
            std::unique_lock<std::mutex> lock(head->m_mtx, std::defer_lock);
            lock.lock();
            if(subpage->free(head, calculateBitmapIdx(handle)))
                return;
            assert(!subpage->m_dontRelase);
            m_subpages[sIdx] = nullptr;
            lock.unlock();
        }

        // free run
        int pages = calculateRunPages(handle);
        std::unique_lock<std::mutex> lock(m_runAvailMtx);

        long concateRun = collapseRuns(handle);
        concateRun &= ~(1L << IS_INUSED_SHIFT_);
        concateRun &= ~(1L << IS_SUBPAGE_SHIFT_);
        insertAvailRun(calculateRunOffset(concateRun), calculateRunPages(concateRun), concateRun);
        m_freeBytes += pages << m_pageShifts;

        if(buffer != nullptr  &&
            m_cachedBuffers.size() < PoolByteBufAllocator::DEFAULT_MAX_CACHED_BYTES_PER_CHUNK)
            m_cachedBuffers.push_back(const_cast<char*>(buffer));

    }

    long PoolChunk::getAvailRunByOffset(int runOffset) {
        if(m_runsAvailMap.find(runOffset) != m_runsAvailMap.end())
            return m_runsAvailMap[runOffset];
        return -1;
    }

    long PoolChunk::collapseRuns(long handle) {
        return collapseNext(collapsePrev(handle));
    }

    long PoolChunk::collapsePrev(long handle) {
        while(true) {
            int runOffset = calculateRunOffset(handle);
            int runPages = calculateRunPages(handle);
            long prevRun = getAvailRunByOffset(runOffset - 1);
//            std::cout << "prevRUn " <<prevRun << " runOffset "<<runOffset<<std::endl;
            if(prevRun == -1) return handle;

            int prevOffset = calculateRunOffset(prevRun);
            int prevPages = calculateRunPages(prevRun);

            //is continuous
            if(prevRun != handle && prevPages + prevOffset == runOffset) {
                removeAvailRun(prevRun);
                handle = toRunHandle(prevOffset, prevPages + runPages, 0);
            } else {
                return handle;
            }

        }
    }

    long PoolChunk::collapseNext(long handle) {
        while(true) {
            int runOffset = calculateRunOffset(handle);
            int runPages = calculateRunPages(handle);
            long nextRun = getAvailRunByOffset(runOffset + runPages);
            if(nextRun == -1) return handle;

            int nextOffset = calculateRunOffset(nextRun);
            int nextPages = calculateRunPages(nextRun);

            //is continuous
            if(nextRun != handle && runPages + runOffset == nextOffset) {
                removeAvailRun(nextRun);
                handle = toRunHandle(runOffset, nextPages + runPages, 0);
            } else {
                return handle;
            }
        }
    }

    void PoolChunk::initBuf(PoolByteBuf* buf, const char * buffer, long handle, int reqCapacity, PoolThreadCache* threadCache) {
        if(isRun(handle))
            buf->init(this, buffer, handle, calculateRunOffset(handle) << m_pageShifts,
                      reqCapacity, runSize(m_pageShifts, handle), m_arena->m_parent->threadCache());
        else
            initBufWithSubpage(buf, buffer, handle, reqCapacity, threadCache);
    }

    void PoolChunk::initBufWithSubpage(PoolByteBuf * buf, const char * buffer, long handle, int reqCapacity,
                                       PoolThreadCache* threadCache) {
        int runOffset = calculateRunOffset(handle);
        int bitmapIdx = calculateBitmapIdx(handle);

        PoolSubpage* subpage = m_subpages[runOffset];
        assert(subpage->m_dontRelase && reqCapacity <= subpage->m_elemSize); //elemSize is normalized subpage unit capacity,1kb, 2kb...

        buf->init(this, buffer, handle, (runOffset << m_pageShifts) + bitmapIdx * subpage->m_elemSize + m_offset,
                  reqCapacity, subpage->m_elemSize, threadCache);
    }

    int PoolChunk::calculateRunOffset(long handle) {
        return static_cast<int>(handle >> RUN_OFFSET_SHIFT_);
    }

    int PoolChunk::calculateRunSize(int pageShifts, long handle) {
        return calculateRunPages(handle) << pageShifts;
    }

    int PoolChunk::calculateRunPages(long handle) {
        return static_cast<int>(handle >> SIZE_SHIFT_ & 0x7fff);
    }

    bool PoolChunk::isUsed(long handle) {
        return (handle >> IS_INUSED_SHIFT_ & 1) == 1L;
    }

    bool PoolChunk::isRun(long handle) {
        return !isSubpage(handle);
    }

    bool PoolChunk::isSubpage(long handle) {
        return (handle >> IS_SUBPAGE_SHIFT_ & 1) == 1L;
    }

    int PoolChunk::calculateBitmapIdx(long handle) {
        return static_cast<int>(handle);
    }

    int PoolChunk::freeBytes() {
        return m_freeBytes;
    }

    int PoolChunk::chunkSize() {
        return m_chunkSize;
    }

}
