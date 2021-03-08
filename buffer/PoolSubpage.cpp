//
// Created by gzp on 2021/2/1.
//

#include <assert.h>
#include "PoolSubpage.h"
#include "PoolArena.h"
#include "PoolChunk.h"

namespace buffer {

    PoolSubpage::PoolSubpage()
    : m_maxNumElems(0), m_bitmapLength(0), m_nextAail(0), m_numAvail(0), prev(nullptr), next(nullptr), m_dontRelase(true)
    {
        m_chunk = nullptr;
        m_pageShifts = -1;
        m_runOffset = -1;
        m_elemSize = -1;
        m_runSize = -1;
    }

    PoolSubpage::PoolSubpage(PoolSubpage * head, PoolChunk * chunk,
                                int pageShifts, int runOffset, int runSize, int elemSize)
                                : prev(nullptr), next(nullptr)
    {
        m_chunk = chunk;
        m_pageShifts = pageShifts;
        m_runSize = runSize;
        m_runOffset = runOffset;
        //m_elemSize是每页被分成单元的大小，如1K，2K，256B，32B等等
        m_elemSize = elemSize;
        m_bitmap.assign(m_runSize >> 6 + SizeClass::LOG2_QUANTUM, 0L );

        m_dontRelase = true;
        if(m_elemSize != 0) {
            m_maxNumElems = m_numAvail = m_runSize / m_elemSize;
            m_nextAail = 0;
            m_bitmapLength = m_maxNumElems >> 6;
            if((m_maxNumElems & 63) != 0) {
                m_bitmapLength ++;
            }

//            for(int i = 0; i < m_bitmapLength; i++) {
//                m_bitmap[i] = 0;
//            }
        }
        addToPool(head);
    }

    long PoolSubpage::allocate()
    {
        if(m_numAvail == 0 || !m_dontRelase)
            return -1;
        int bitmapIdx = getNextAvail();
        int q = bitmapIdx >> 6;
        int r = bitmapIdx & 63;
        assert((m_bitmap[q] >> r & 1) == 0);
        m_bitmap[q] |= (1L << r);
        if(-- m_numAvail == 0) {
            removeFromPool();
        }

        return toHandle(bitmapIdx);
    }

    bool PoolSubpage::free(PoolSubpage * head, int bitmapIdx) {
        if(m_elemSize == 0)
            return true;
        int q = bitmapIdx >> 6;
        int r = bitmapIdx & 63;
        assert((m_bitmap[q] >> r & 1) != 0);
        m_bitmap[q] ^= 1L << r;
        setNextAvail(bitmapIdx);
        if(m_numAvail ++ == 0) {
            addToPool(head);
            return true;
        }
        if(m_numAvail != m_maxNumElems) {
            return true;
        }
        else {
            if(prev == next)
                return true;

            m_dontRelase = false;
            removeFromPool();
            return false;
        }
    }

    void PoolSubpage::addToPool(PoolSubpage * head) {
        assert(prev == nullptr && next == nullptr);
        prev = head;
        next = head->next;
        next->prev = this;
        head->next = this;
    }

    void PoolSubpage::removeFromPool() {
        assert(prev != nullptr && next != nullptr);
        prev->next = next;
        next->prev = prev;
        prev = nullptr;
        next = nullptr;
    }

    void PoolSubpage::setNextAvail(int bitmapIdx) {
        m_nextAail = bitmapIdx;
    }

    int PoolSubpage::getNextAvail() {
        int nextAvail = m_nextAail;
        if(nextAvail >= 0) {
            m_nextAail = -1;
            return nextAvail;
        }
        return findNextAvail();
    }

    int PoolSubpage::findNextAvail() {
//        long & bitmap = m_bitmap;
        int bitmapLength = m_bitmapLength;
        for(int i = 0; i < bitmapLength; i ++) {
            long bits = m_bitmap[i];
            if(~bits != 0)
                return findNextAvail0(i, bits);
        }
        return -1;
    }

    int PoolSubpage::findNextAvail0(int i, long bits) {
//        int maxNumElems = m_maxNumElems;
        int baseVal = i << 6;
        for(int j = 0; j < 64; j ++) {
            if((bits & 1) == 0) {
                int val = baseVal | j;
                if(val < m_maxNumElems) {
                    return val;
                } else {
                    break;
                }
            }
            bits >>= 1;
        }
        return -1;
    }

    long PoolSubpage::toHandle(int bitmapIdx) {
        int pages = m_runSize >> m_pageShifts;
        return static_cast<long>(m_runOffset) << PoolChunk::RUN_OFFSET_SHIFT_
                | static_cast<long>(pages) << PoolChunk::SIZE_SHIFT_
                | 1L << PoolChunk::IS_INUSED_SHIFT_
                | 1L << PoolChunk::IS_SUBPAGE_SHIFT_
                | bitmapIdx;

    }

}
